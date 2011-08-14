/*

    DownloadManager - http://www.excito.com/

    TorrentDownloader.cpp - this file is part of DownloadManager.

    Copyright (C) 2007 Tor Krill <tor@excito.com>

    DownloadManager is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    DownloadManager is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    version 2 along with DownloadManager; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.

    $Id: TorrentDownloader.cpp 2026 2008-10-16 10:12:57Z tor $
*/

#include <sys/select.h>
#include <errno.h>
#include <pwd.h>
#include <sys/types.h>
#include <grp.h>
#include <stdio.h>
#include <syslog.h>
#include <time.h>

#include <stdexcept>
#include <iostream>
#include <sstream>
#include <algorithm>

#include <typeinfo>

#include <ext/stdio_filebuf.h>

#include "FtdConfig.h"
#include <libeutils/FileUtils.h>
#include <libeutils/UserGroups.h>
#include "DirWatcher.h"
#include <libeutils/EExcept.h>
#include <libeutils/StringTools.h>
#include <libeutils/DeferredWork.h>
#include "TorrentDownloader.h"
#include <libeutils/json/json.h>

#include <libtorrent/version.hpp>
#include <libtorrent/entry.hpp>
#include <libtorrent/bencode.hpp>
#include <libtorrent/torrent_info.hpp>
#include <libtorrent/alert_types.hpp>

#include <libtorrent/extensions/metadata_transfer.hpp>
#include <libtorrent/extensions/ut_pex.hpp>

#include <boost/filesystem/operations.hpp>

#ifdef DMALLOC
#include <dmalloc.h>
#endif

using namespace libtorrent;
using namespace boost::filesystem;

/*******************************************
*
* 	Implementation of TorrentDownloader
*
*
*******************************************/

TorrentDownloader::TorrentDownloader(TorrentDownloadManager* mgr):Downloader(){
	this->mgr=mgr;
	this->deletetorrent=true;
	this->perm_changed=false;
	this->downloader=NULL;
}

TorrentDownloader::~TorrentDownloader(){


	switch(this->status){
	case DOWNLOADED:
		{
			// Manually remove download, a bit of code duplication, consider refactor do_cancel
			this->mgr->GetSession().remove_torrent(this->handle);
			this->mgr->UnregisterHandle(this->handle);
			break;
		}
	case INPROGRESS:
	case NOTSTARTED:
	case QUEUED:
		{
			this->CancelDownload();
			this->WaitForCompletion();
			break;
		}
	case CANCELINPROGRESS:
		{
			this->WaitForCompletion();
			break;
		}
	case CANCELDONE:
	case FAILED:
		break;
	default:
		syslog(LOG_NOTICE,"Destroying download in unknown state %d",this->status);
	}

	this->mgr->RemoveDownload(this);

	if(this->deletetorrent){
		this->DeleteTorrent();
	}
}

void TorrentDownloader::DeleteTorrent(void){
	// Remove torrent
	if(this->torrentfilename.length()>0){
		try{
			if(unlink(this->torrentfilename.c_str())){
				syslog(LOG_WARNING,"Failed to remove torrent file %s because %m"
						,this->torrentfilename.c_str());
			}
		}catch(EExcept::ENoent &err){
			syslog(LOG_WARNING,"File %s does not excist",this->torrentfilename.c_str());
		}catch(...){
			syslog(LOG_ERR,"Unknown exception caught when deleting %s"
					,this->torrentfilename.c_str());
		}
	}
	// Remove possible resume data
	if(this->resumefilename.length()>0){
		try{
			if(unlink(this->resumefilename.c_str())){
				syslog(LOG_WARNING,"Failed to remove resume file %s because %m"
						,this->resumefilename.c_str());
			}
		}catch(EExcept::ENoent &err){
			syslog(LOG_WARNING,"File %s does not excist",this->resumefilename.c_str());
		}catch(...){
			syslog(LOG_ERR,"Unknown exception caught when deleting %s"
					,this->resumefilename.c_str());
		}
	}

}

void TorrentDownloader::SetDeletestatus(bool dodelete){
	this->deletetorrent=dodelete;
}
void TorrentDownloader::DoWriteResumeData(boost::shared_ptr<entry> resume_data){
	syslog(LOG_DEBUG,"Do write resume data");

	if(!this->handle.is_valid()){
		syslog(LOG_NOTICE,"Handle not valid, not writing resume data");
		return;
	}

	// Find directory to write to
	string wp="/home/"+User::UIDToUser(this->user);
	string suf="/"+FtdConfig::Instance().GetStringOrDefault("torrent","resumepath","torrents/.resumedata");

	if(!Stat::DirExists(wp+suf)){
		syslog(LOG_INFO,"Creating resume data directory");
		FileUtils::MkPath(wp+suf);
		FileUtils::Chown(wp+suf,this->user,"users");
	}
	this->resumefilename=wp+suf+"/"+StringTools::GetFileName(this->torrentfilename)+".resume";
	std::ofstream of(this->resumefilename.c_str(),ios::trunc|ios::out);
	std::ostream_iterator<char> ofIt(of);
	bencode(ofIt,*resume_data);
	of.close();
	FileUtils::Chown(this->resumefilename,this->user,"users");


}

void TorrentDownloader::WriteResumeData(void){
	syslog(LOG_DEBUG,"Initiate write resume data");

	if(!this->handle.is_valid()){
		syslog(LOG_NOTICE,"Handle not valid, not writing resume data");
		return;
	}

	this->handle.pause();

	this->handle.save_resume_data();
	this->m_resumewritten.Wait();
}

void TorrentDownloader::WriteTorrent(void){

	// Do some sanity checks
	// TODO: evaluate if this is the right thing to do
	if(!this->status==FAILED){
		syslog(LOG_NOTICE,"Torrent had failures, not writing torrent data");
		return;
	}

	// Find directory to write to
	string wp="/home/"+User::UIDToUser(this->user);
	string suf="/torrents/";
	try{
		FtdConfig& cfg=FtdConfig::Instance();
		suf="/"+cfg.GetString("torrent","torrentdir")+"/";
	}catch(std::runtime_error& err){
		syslog(LOG_ERR,"Unable to read torrent dir");
	}

	if(!Stat::DirExists(wp+suf)){
		FileUtils::MkPath(wp+suf);
		FileUtils::Chown(wp+suf,this->user,"users");
		DirWatcher::Instance().AddWatch(wp+suf);
	}

	string fn=wp+suf+".ftdtrXXXXXX";

	int fd;
	char *pt=(char*)malloc(fn.length()+1);
	if(!pt){
		syslog(LOG_CRIT,"Failed to allocate memory for path %s because %m"
				,wp.c_str());
		return;
	}
	sprintf(pt,"%s",fn.c_str());
	if((fd=mkstemp(pt))<0){
		syslog(LOG_ERR,"Failed to create torrent file in %s because %m",fn.c_str());
		return;
	}

	this->torrentfilename=pt;
	FILE* file=fdopen(fd,"w");
	// This is definetly not portable....
	__gnu_cxx::stdio_filebuf<char> fb(file,std::ios_base::out);
	iostream fil(&fb);

	fil<< m_dlstream.str();
	fil.flush();
	fb.close();

	// Rewind stream for output later use
	m_dlstream.seekg (0, ios::beg);


	uid_t user=this->user==0xffff?0:this->user;
	gid_t group=this->group==0xffff?0:this->group;
	if(chown(pt,user,group)){
		syslog(LOG_NOTICE,"Failed to chown file %s because %m",wp.c_str());
	}
	free(pt);
	close(fd);
}

bool TorrentDownloader::StartFromFile(const string& path){
	syslog(LOG_INFO,"Adding download from file %s",path.c_str());
	this->torrentfilename=path;
	std::filebuf fb;
	fb.open(path.c_str(),ios::in);
	iostream infile(&fb);
	this->SetUUID(StringTools::SimpleUUID());
	return this->AddDownload(&infile);
}

bool TorrentDownloader::AddDownload(std::iostream* str){

	//TODO: inform user on failure, design idea, add return value and check
	//      Refactor HttpFailed to be called something like AddDownloadFailed
	//      and call that if this function returns false.
	bool result=true;

	syslog(LOG_INFO,"Adding download to session");
	str->unsetf(std::ios_base::skipws);
	try{
		std::vector<char> iost;
		std::copy(std::istream_iterator<char>(*str),std::istream_iterator<char>(),
				std::back_inserter(iost));
		torrent_info* ti=new torrent_info(&iost[0],iost.size());

		this->SetDownloadName(ti->name());
		this->UpdateProgress(ti->total_size(),0,0,0);

		std::vector<char>* resume_data=NULL;
		this->resumefilename="/home/"+User::UIDToUser(this->user)
			+"/"+FtdConfig::Instance().GetStringOrDefault("torrent","resumepath","torrents/.resumedata")
			+"/"+StringTools::GetFileName(this->torrentfilename)+".resume";

		if(Stat::FileExists(this->resumefilename)){
			syslog(LOG_INFO,"Adding resume data from file");
			std::ifstream inf(this->resumefilename.c_str());
			inf.unsetf(std::ios_base::skipws);
			istream_iterator<char> ifIt(inf);
			resume_data=new std::vector<char>();
			std::copy(ifIt,istream_iterator<char>(),std::back_inserter(*resume_data));
			inf.close();

		}else{
			syslog(LOG_INFO,"Resumedata not found");
		}
#if 1
		//TODO: use when porting to non deprecated API
		add_torrent_params atp;
		atp.ti=ti;
		atp.save_path=this->destinationpath;
		atp.duplicate_is_error=true;
		atp.resume_data=resume_data;
		atp.paused=false;
		this->handle=this->mgr->GetSession().add_torrent(atp);
#endif
		if(resume_data){
			delete resume_data;
		}
#if 0
		this->handle=this->mgr->GetSession().add_torrent(
				ti,
				this->destinationpath,
				resume,
				libtorrent::storage_mode_allocate,
				false,
				default_storage_constructor
				);
#endif

		this->mgr->RegisterHandle(this->handle,this);
		// TODO: Maybe read this from config
		this->handle.set_max_connections(40);
		this->handle.set_max_uploads(-1);
#if LIBTORRENT_VERSION_MINOR > 13
		this->handle.set_sequential_download(false);
#else
		this->handle.set_sequenced_download_threshold(15);
#endif
	}catch(libtorrent_exception& e){
		boost::system::error_code ec = e.error();
		syslog(LOG_ERR,ec.message().c_str());
		this->errmsg = ec.message();
		this->status = FAILED;
		this->SignalFailed.emit(ec.message());
		this->Complete();
	}
	return result;
}


void TorrentDownloader::HttpDone() {
	syslog(LOG_INFO,"Torrent download done");
	if(this->downloader){
		this->WriteTorrent();
		this->AddDownload(this->downloader->GetStream());
		DeferredWork::Instance().AddWork(TorrentDownloader::DeferredCleanup,this);
	}else{
		syslog(LOG_CRIT,"Panic, no http downloader...");
	}

}

void TorrentDownloader::DeferredCleanup(void*pl){
	const struct timespec delay = {0,100000000L};
	TorrentDownloader* dl=static_cast<TorrentDownloader*>(pl);
	syslog(LOG_DEBUG,"Doing deferred CurlDownload cleanup");
	if(dl->downloader){
		CurlDownloader* hdl=dl->downloader;
		dl->downloader=NULL;
		while(!hdl->ReapOK()){
			nanosleep(&delay,NULL);
		}
		delete hdl;
	}
	syslog(LOG_DEBUG,"Done deferred torrent cleanup");
}

void TorrentDownloader::HttpFailed(const string &msg){

	syslog(LOG_ERR,"Torrent download failed: %s",msg.c_str());

	this->errmsg=msg;
	this->status=FAILED;

	if(this->downloader){
		DeferredWork::Instance().AddWork(TorrentDownloader::DeferredCleanup,this);
	}

	this->DeleteTorrent();

	this->SignalFailed.emit(msg);

	this->Complete();
}

void TorrentDownloader::Update(){
	try{
		if(this->handle.is_valid()){
			torrent_status ts=this->handle.status();

			if(this->UpdateProgress(ts.total_wanted,ts.total_wanted_done,0,0)){
				// Cancel download?
			}
		}
	}catch(invalid_handle& ih){

	}
}

void TorrentDownloader::DownloadDone(){
	syslog(LOG_INFO,"Download finished: %s",this->handle.name().c_str());

	this->status=DOWNLOADED;
	this->tip=false;

	this->Complete();
	this->SignalDone.emit();

}

void TorrentDownloader::ChangePermissions(){

	if ( !this->perm_changed && ((this->user!=0xffff) || (this->group!=0xffff)) ) {
		this->perm_changed=true;
		uid_t user=this->user==0xffff?0:this->user;
		gid_t group=this->group==0xffff?0:this->group;

		if(!this->handle.is_valid()){
			syslog(LOG_NOTICE,"Torrent handle invalid, skip chown files");
			return;
		}

		torrent_info ti=this->handle.get_torrent_info();
		for(torrent_info::file_iterator fIt=ti.begin_files();fIt!=ti.end_files();fIt++){
			string filename=(*fIt).path.native_file_string();

			//syslog(LOG_DEBUG,"File: %s",(this->destinationpath+"/"+filename).c_str());

			// Chown possible directories
			int pos=0;
			while((pos=filename.find("/",pos))>=0){
				if(chown((this->destinationpath+"/"+filename.substr(0,pos)).c_str(),user,group)){
					syslog(LOG_NOTICE,"Torrent chown directory failed: %s, %m "
							,(this->destinationpath+"/"+filename.substr(0,pos)).c_str());
				}
				pos++;
			}

			// Chown file it self
			if ( chown((this->destinationpath+"/"+filename).c_str(),user,group) ) {
				syslog(LOG_NOTICE,"Torrent Chown failed: %m");
			}
		}
	}
}

void TorrentDownloader::DoCancel(){

	if(this->handle.is_valid()){
		syslog(LOG_INFO,"Removing torrent from session");
		this->mgr->GetSession().remove_torrent(this->handle);
		this->mgr->UnregisterHandle(this->handle);
	}else{
		syslog(LOG_ERR,"Doing cancel on invalid torrent");
	}

	this->status=CANCELDONE;
	this->Complete();

}

void TorrentDownloader::DeferredCancel(void *pl){
	TorrentDownloader* dl=static_cast<TorrentDownloader*>(pl);
	dl->DoCancel();
}

void TorrentDownloader::CancelDownload(){

	syslog(LOG_DEBUG,"Cancel download");

	// Make sure downloaded files is accessible by user
	// TODO: This is a workaround for us not being aware when a file is
	// 		created by libtorrent
	this->ChangePermissions();

	// If aint done we have failed with this download.

	if (this->status==CANCELINPROGRESS || this->status==CANCELDONE || this->status==FAILED) {
		syslog(LOG_DEBUG,"CancelDownload not adding deferred cancel since state is %d",this->status);
		return;
	}

	// Dont start a cancel on a downloaded torrent. Destructor will clean this up
	if( this->status != DOWNLOADED){

		this->status=CANCELINPROGRESS;
		this->cancel=true;

		DeferredWork::Instance().AddWork(DeferredCancel,this);
	}

}

void TorrentDownloader::StopDownload(void){
	syslog(LOG_DEBUG,"Stopping download");

	// Only keep torrent if its valid
	if(this->handle.is_valid()){
		this->deletetorrent=false;
	}

	this->WriteResumeData();
	this->CancelDownload();
}

void TorrentDownloader::SetInfo(string info){
}

/*
static void DumpSessionInfo(session& s){
	syslog(LOG_DEBUG,"---- General info ----");
	syslog(LOG_DEBUG,"upload_rate_limit   : %d",s.upload_rate_limit());
	syslog(LOG_DEBUG,"download_rate_limit : %d",s.download_rate_limit());
	syslog(LOG_DEBUG,"num_uploads         : %d",s.num_uploads());
	syslog(LOG_DEBUG,"num_connections     : %d",s.num_connections());
	syslog(LOG_DEBUG,"is_listening        : %d",s.is_listening());
	syslog(LOG_DEBUG,"listen_port         : %d",s.listen_port());
	syslog(LOG_DEBUG,"---- Session status ----");
	session_status st=s.status();
	syslog(LOG_DEBUG,"has_incoming_connections: %d",st.has_incoming_connections);
	syslog(LOG_DEBUG,"upload_rate             : %f",st.upload_rate);
	syslog(LOG_DEBUG,"download_rate           : %f",st.download_rate);
	syslog(LOG_DEBUG,"payload_upload_rate     : %f",st.payload_upload_rate);
	syslog(LOG_DEBUG,"payload_download_rate   : %f",st.payload_download_rate);
	syslog(LOG_DEBUG,"total_download          : %d",st.total_download);
	syslog(LOG_DEBUG,"total_upload            : %d",st.total_upload);
	syslog(LOG_DEBUG,"total_payload_download  : %d",st.total_payload_download);
	syslog(LOG_DEBUG,"total_payload_upload    : %d",st.total_payload_upload);
	syslog(LOG_DEBUG,"num_peers               : %d",st.num_peers);
	syslog(LOG_DEBUG,"---- Alerts -----");

	bool done=false;
	do{
		std::auto_ptr<alert> alrt=s.pop_alert();
		if(alrt.get()!=0){
			syslog(LOG_DEBUG,"Alert: %s",alrt->msg().c_str());
		}else{
			done=true;
		}

	}while(!done);
}
*/

Json::Value TorrentDownloader::GetInfo(){
	Json::Value ret(Json::objectValue);
	ret["type"]="torrent";
	try{
		torrent_status ts=this->handle.status();

		ret["paused"]=ts.paused;
		ret["progress"]=ts.progress;

		ret["current_tracker"]=ts.current_tracker;

		ret["total_download"]=static_cast<long long unsigned int>(ts.total_download);
		ret["total_upload"]=static_cast<long long unsigned int>(ts.total_upload);

		ret["total_payload_download"]=static_cast<long long unsigned int>(ts.total_payload_download);
		ret["total_payload_upload"]=static_cast<long long unsigned int>(ts.total_payload_upload);

		ret["total_failed_bytes"]=static_cast<long long unsigned int>(ts.total_failed_bytes);
		ret["total_redundant_bytes"]=static_cast<long long unsigned int>(ts.total_redundant_bytes);

		ret["download_rate"]=ts.download_rate;
		ret["upload_rate"]=ts.upload_rate;

		ret["download_payload_rate"]=ts.download_payload_rate;
		ret["upload_payload_rate"]=ts.upload_payload_rate;
		ret["num_peers"]=ts.num_peers;
		ret["num_complete"]=ts.num_complete;
		ret["num_incomplete"]=ts.num_incomplete;
		ret["list_seeds"]=ts.list_seeds;
		ret["list_peers"]=ts.list_peers;
		ret["num_pieces"]=ts.num_pieces;
		ret["total_done"]=static_cast<long long unsigned int>(ts.total_done);
		ret["total_wanted_done"]=static_cast<long long unsigned int>(ts.total_wanted_done);
		ret["total_wanted"]=static_cast<long long unsigned int>(ts.total_wanted);
		ret["num_seeds"]=ts.num_seeds;
		ret["distributed_copies"]=ts.distributed_copies;
		ret["block_size"]=ts.block_size;
		ret["num_uploads"]=ts.num_uploads;
		ret["num_connections"]=ts.num_connections;
		ret["uploads_limit"]=ts.uploads_limit;
		ret["connections_limit"]=ts.connections_limit;
		ret["up_bandwidth_queue"]=ts.up_bandwidth_queue;
		ret["down_bandwidth_queue"]=ts.down_bandwidth_queue;

		switch(ts.state){
		case torrent_status::queued_for_checking:
			ret["state"]="queued_for_checking";
			break;
		case torrent_status::checking_files:
			ret["state"]="checking_files";
			break;
		case torrent_status::downloading_metadata:
			ret["state"]="downloading_metadata";
			break;
		case torrent_status::downloading:
		{
			this->status=INPROGRESS;
			ret["state"]="downloading";
			break;
		}
		case torrent_status::finished:
			ret["state"]="finished";
			break;
		case torrent_status::seeding:
		{
			this->status=DOWNLOADED;
			ret["state"]="seeding";
			break;
		}
		case torrent_status::allocating:
			ret["state"]="allocating";
			break;
		case torrent_status::checking_resume_data:
			ret["state"]="checking_resume_data";
			break;
		default:
			ret["state"]="unknown";
			break;
		}
	}catch(invalid_handle& ih){
		if(this->status==FAILED){
			ret["state"]="failed";
			ret["errmsg"]=this->errmsg;
		}else{
			ret["state"]="no_info";
		}
	}
	return ret;
}


void TorrentDownloader::StartDownload(){

	if(this->url.Scheme()=="magnet"){
		syslog(LOG_INFO,"Adding magnet link");

	}else{
		syslog(LOG_INFO,"Starting download of torrent");

		downloader=CurlDownloadManager::Instance().GetDownloader();

		downloader->SignalDone.connect(sigc::mem_fun(this,&TorrentDownloader::HttpDone));
		downloader->SignalFailed.connect(sigc::mem_fun(this,&TorrentDownloader::HttpFailed));
		downloader->SetUrl(this->GetUrl());
		downloader->SetStream(&m_dlstream);

		this->status=QUEUED;
		this->tip=true;

		downloader->StartDownload();
	}
}

/*
* 	Implementation of TorrentDownloadManager
*
*
*/

TorrentDownloadManager::TorrentDownloadManager():
	s(libtorrent::fingerprint("LT", 0, 1, 0, 0),0)
{
	// Start workerthread
	this->Start();

}

static const char* statetostring(torrent_status::state_t st){
	switch(st){
	case torrent_status::queued_for_checking:
		return "queued_for_checking";
	case torrent_status::checking_files:
		return "checking_files";
	case torrent_status::downloading_metadata:
		return "downloading_metadata";
	case torrent_status::downloading:
		return "downloading";
	case torrent_status::finished:
		return "finished";
	case torrent_status::seeding:
		return "seeding";
	case torrent_status::allocating:
		return "allocating";
	case torrent_status::checking_resume_data:
		return "checking_resume_data";
	default:
		return "Unknown state";
	}
}

void TorrentDownloadManager::HandleAlerts(){
	bool done=false;
	do{
		std::auto_ptr<alert> alrt=this->s.pop_alert();
		alert* a=alrt.get();
		if(a!=0){

			if(typeid(*a)==typeid(listen_failed_alert)){

				syslog(LOG_CRIT,"Not able to open listen port: %s",alrt->what());
			}
			else if(typeid(*a)==typeid(portmap_error_alert)){

				syslog(LOG_ERR, "Portmap error: %s", alrt->what());

			}else if(typeid(*a)==typeid(portmap_alert)){

			}
#if 0
			//TODO: not supported by libtorrent in jaunty
			else if(typeid(*a)==typeid(portmap_log_alert)){
				portmap_log_alert* pla=dynamic_cast<portmap_log_alert*>(a);
				syslog(LOG_INFO, "Portmap alert: (%d) %s",pla->type,pla->msg.c_str());
			}
#endif
			else if(typeid(*a)==typeid(file_error_alert)){

				// Read or write for torrent failed.
				// TODO: implement some notification
				syslog(LOG_CRIT,"File errror: %s",alrt->what());

			}else if(typeid(*a)==typeid(file_renamed_alert)){

				syslog(LOG_DEBUG,"Filname renamed alert");

			}else if(typeid(*a)==typeid(file_rename_failed_alert)){

				syslog(LOG_DEBUG,"Filname renamed error alert");

			}else if(typeid(*a)==typeid(tracker_announce_alert)){

				syslog(LOG_INFO,"Tracker announce sent");

				tracker_announce_alert* taa=dynamic_cast<tracker_announce_alert*>(a);

				// Notify downloader that we should be up and running
				TorrentDownloader* td=this->FindDownloader(taa->handle);
				if(td){
					td->ChangePermissions();
				}else{
					syslog(LOG_ERR,"Change permissions, downloader not found");
				}

			}else if(typeid(*a)==typeid(tracker_error_alert)){

				tracker_error_alert* ta=dynamic_cast<tracker_error_alert*>(a);
				syslog(LOG_NOTICE,"Tracker announce failed: %s",ta->what());

			}else if(typeid(*a)==typeid(tracker_reply_alert)){

				tracker_reply_alert* tra=dynamic_cast<tracker_reply_alert*>(a);
				syslog(LOG_INFO,"Tracker announce success, peers: %d",tra->num_peers);

			}else if(typeid(*a)==typeid(tracker_warning_alert)){

				tracker_warning_alert* twa=dynamic_cast<tracker_warning_alert*>(a);
				syslog(LOG_WARNING,"Tracker warning: %s",twa->what());

			}else if(typeid(*a)==typeid(scrape_reply_alert)){

				syslog(LOG_INFO,"Scrape reply");

			}else if(typeid(*a)==typeid(scrape_failed_alert)){

				syslog(LOG_NOTICE,"Scrape failed");

			}else if(typeid(*a)==typeid(url_seed_alert)){

				url_seed_alert* usa=dynamic_cast<url_seed_alert*>(a);
				syslog(LOG_WARNING,"Url seed alert, url: %s",usa->url.c_str());

			}else if(typeid(*a)==typeid(hash_failed_alert)){

				// hash_failed_alert* hfa=dynamic_cast<hash_failed_alert*>(a);
				syslog(LOG_INFO,"Hash failed for torrent");

			}else if(typeid(*a)==typeid(peer_ban_alert)){

				// peer_ban_alert* pba=dynamic_cast<peer_ban_alert*>(a);
				syslog(LOG_INFO,"Peer ban alert");

			}else if(typeid(*a)==typeid(peer_error_alert)){

				// peer_error_alert* pea=dynamic_cast<peer_error_alert*>(a);
				syslog(LOG_DEBUG,"Peer error, invalid data received");

			}else if(typeid(*a)==typeid(invalid_request_alert)){

				//invalid_request_alert* ira=dynamic_cast<invalid_request_alert*>(a);
				syslog(LOG_DEBUG,"Invalid piece request");

			}else if(typeid(*a)==typeid(torrent_finished_alert)){

				torrent_finished_alert* tfa=dynamic_cast<torrent_finished_alert*>(a);

				this->dlmutex.Lock();
				map<torrent_handle,TorrentDownloader*>::iterator mIt;
				mIt=this->handle_map.find(tfa->handle);
				if(mIt!=this->handle_map.end()){
					(*mIt).second->DownloadDone();
				}
				this->dlmutex.Unlock();

			}else if(typeid(*a)==typeid(performance_alert)){

				syslog(LOG_DEBUG,"Performance alert: %s",alrt->what());

			}else if(typeid(*a)==typeid(state_changed_alert)){

				state_changed_alert* sca=dynamic_cast<state_changed_alert*>(a);
				syslog(LOG_DEBUG,"State changed: %s",statetostring(sca->state));

			}else if(typeid(*a)==typeid(metadata_failed_alert)){

				//metadata_failed_alert* mfa=dynamic_cast<metadata_failed_alert*>(a);
				syslog(LOG_INFO,"Metadata failed");


			}else if(typeid(*a)==typeid(metadata_received_alert)){

				//metadata_received_alert* mra=dynamic_cast<metadata_received_alert*>(a);
				syslog(LOG_INFO,"Metadata received");

			}else if(typeid(*a)==typeid(fastresume_rejected_alert)){

				//fastresume_rejected_alert* fra=dynamic_cast<fastresume_rejected_alert*>(a);
				syslog(LOG_INFO,"Fast resume rejected");

			}else if(typeid(*a)==typeid(peer_blocked_alert)){

				syslog(LOG_INFO,"Peer blocked");

			}else if(typeid(*a)==typeid(storage_moved_alert)){

				syslog(LOG_INFO,"Storage move complete");

			}else if(typeid(*a)==typeid(torrent_paused_alert)){

				syslog(LOG_DEBUG,"Torrent paused: %s",alrt->what());

			}else if(typeid(*a)==typeid(torrent_resumed_alert)){

				syslog(LOG_DEBUG,"Torrent resumed: %s",alrt->what());

			}else if(typeid(*a)==typeid(save_resume_data_alert)){

				syslog(LOG_DEBUG,"Save resume: %s",alrt->what());
				save_resume_data_alert* srda=dynamic_cast<save_resume_data_alert*>(a);

				TorrentDownloader* td=this->FindDownloader(srda->handle);
				if(td){
					td->DoWriteResumeData(srda->resume_data);
					td->m_resumewritten.Notify();
				}else{
					syslog(LOG_ERR,"Save resume data failed, downloader not found");
				}

			}else if(typeid(*a)==typeid(save_resume_data_failed_alert)){

				syslog(LOG_ERR,"Save resume failed: %s",alrt->what());
				save_resume_data_failed_alert* srdf=dynamic_cast<save_resume_data_failed_alert*>(a);
				TorrentDownloader* td=this->FindDownloader(srdf->handle);
				if(td){
					td->m_resumewritten.Notify();
				}else{
					syslog(LOG_ERR,"Save resume data failed AND downloader not found");
				}


			}else if(typeid(*a)==typeid(external_ip_alert)){

				external_ip_alert* eia=dynamic_cast<external_ip_alert*>(a);
				syslog(LOG_DEBUG,"External IP received: %s",eia->external_address.to_string().c_str());

			}else{

				syslog(LOG_DEBUG,"Unknown alert: %s",alrt->what());
			}
		}else{
			done=true;
		}
	}while(!done);

}

void TorrentDownloadManager::UpdateDownloaders(){
	this->dlmutex.Lock();

	for(list<TorrentDownloader*>::iterator lIt=this->downloaders.begin();
		lIt!=this->downloaders.end();++lIt){
		(*lIt)->Update();
	}

	this->dlmutex.Unlock();

}

void TorrentDownloadManager::Run(){

	this->isRunning=true;

	syslog(LOG_NOTICE,"TorrentDownloadManager starting up");

	// Set verboseness of libtorrent
	int alertlvl = alert::all_categories & ~alert::debug_notification;

	alertlvl &= ~alert::progress_notification;

	this->s.set_alert_mask(alertlvl);

	// Get default config
	FtdConfig& cfg=FtdConfig::Instance();

	// Set config
	session_settings settings;
	settings.user_agent=USER_AGENT;
	if(cfg.GetStringOrDefault("general","proxyip","")!=""){
#ifdef SARGE
		settings.proxy_ip=cfg.GetStringOrDefault("general","proxyip","");
		settings.proxy_port=cfg.GetIntegerOrDefault("general","proxyport",0);
		settings.proxy_login=cfg.GetStringOrDefault("general","proxylogin","");
		settings.proxy_password=cfg.GetStringOrDefault("general","proxypassword","");
#endif
#ifdef ETCH
		//TODO: Implement proxy settings
#endif
	}

	if(cfg.GetBoolOrDefault("torrent","metadatasupport",true)){
		syslog(LOG_INFO,"Adding metadata support to session");
		this->s.add_extension(&libtorrent::create_metadata_plugin);
	}else{
		syslog(LOG_DEBUG,"Not adding metadatasupport");
	}

	if(cfg.GetBoolOrDefault("torrent","peerexchangesupport",true)){
		syslog(LOG_INFO,"Adding peer exchange support to session");
		this->s.add_extension(&libtorrent::create_ut_pex_plugin);
	}else{
		syslog(LOG_DEBUG,"Not adding peer exchange support");
	}


	if(cfg.GetBoolOrDefault("torrent","dhtsupport",false)){
		settings.use_dht_as_fallback = false;
		dht_settings dht_s;
		dht_s.service_port=cfg.GetIntegerOrDefault("torrent","listenportstart",10000);
		this->s.set_dht_settings(dht_s);

		lazy_entry e;

		string stpath=cfg.GetStringOrDefault("general","statedir","/etc/ftd")+"/dhtstate";
		if(Stat::FileExists(stpath)){
			try{
				int size = file_size(stpath);
                if (size > 10 * 1000000)
                {
                        return;
                }
				std::vector<char> buf(size);
                std::ifstream(stpath.c_str(), std::ios_base::binary).read(&buf[0], size);
				lazy_bdecode(&buf[0], &buf[0] + buf.size(), e);

			}catch(libtorrent_exception& e){
				syslog(LOG_ERR,e.error().message().c_str());
			}

		}
		syslog(LOG_INFO,"Starting dht support");
		this->s.load_state(e);
		this->s.start_dht();
		this->s.add_dht_router(std::make_pair(std::string("router.bittorrent.com"),6881));
		this->s.add_dht_router(std::make_pair(std::string("router.utorrent.com"),6881));
		this->s.add_dht_router(std::make_pair(std::string("router.bitcomet.com"),6881));
	}else{
		syslog(LOG_DEBUG,"Not starting dht support");
	}

	this->s.set_max_uploads(cfg.GetIntegerOrDefault("torrent","maxuploads",8));
	this->s.set_max_half_open_connections(cfg.GetIntegerOrDefault("torrent","maxhalfopenconnections",-1));


	int startport=cfg.GetIntegerOrDefault("torrent","listenportstart",10000);
	int endport=cfg.GetIntegerOrDefault("torrent","listenportend",14000);

	this->s.set_settings(settings);

	if(!this->s.listen_on(make_pair(startport,endport))){
		syslog(LOG_CRIT,"Could not open a listening port.");
		throw std::runtime_error("Could not open a listening port.");
	}

	int downloadthrottle=cfg.GetIntegerOrDefault("torrent","maxdownload",0);
	int uploadthrottle=cfg.GetIntegerOrDefault("torrent","maxupload",0);

	this->SetDownloadThrottle(downloadthrottle);
	this->SetUploadThrottle(uploadthrottle);

	DirWatcher::Instance().Start();

	while(this->isRunning){
		this->HandleAlerts();
		this->UpdateDownloaders();
		sleep(1);
	}

}

void TorrentDownloadManager::RegisterHandle(torrent_handle h,TorrentDownloader *td){
	this->dlmutex.Lock();
	this->handle_map[h]=td;
	this->dlmutex.Unlock();
}

TorrentDownloader* TorrentDownloadManager::FindDownloader(torrent_handle h){
	TorrentDownloader *td=NULL;
	this->dlmutex.Lock();

	map<torrent_handle,TorrentDownloader*>::iterator mIt;
	mIt=this->handle_map.find(h);
	if(mIt!=this->handle_map.end()){
		td=(*mIt).second;
	}

	this->dlmutex.Unlock();
	return td;
}


void TorrentDownloadManager::UnregisterHandle(torrent_handle h){
	this->dlmutex.Lock();
	if(!this->handle_map.erase(h)){
		try{
			syslog(LOG_ERR,"Erase of handle %s failed",h.name().c_str());
		}catch(libtorrent::invalid_handle& err){
			syslog(LOG_ERR,"Erase of invalid handle");
		}
	}
	this->dlmutex.Unlock();
}

void TorrentDownloadManager::RemoveDownload(TorrentDownloader* t){
	this->dlmutex.Lock();
	list<TorrentDownloader*>::iterator elem=find(
			this->downloaders.begin(),
			this->downloaders.end(),
			t
			);
	if(elem!=this->downloaders.end()){
		this->downloaders.erase(elem);
	}else{
		syslog(LOG_ERR,"Trying to remove non registered download");
	}
	this->dlmutex.Unlock();
}

TorrentDownloadManager& TorrentDownloadManager::Instance(){
	static TorrentDownloadManager mgr;

	return mgr;
}

session& TorrentDownloadManager::GetSession(){
	return this->s;
}

TorrentDownloader* TorrentDownloadManager::NewTorrentDownloader(){
	TorrentDownloader* dl=new TorrentDownloader(this);

	this->dlmutex.Lock();
	this->downloaders.push_back(dl);
	this->dlmutex.Unlock();

	return dl;
}

Downloader* TorrentDownloadManager::Filter(const URL& url,map<string,string>& hints){
	Downloader* dl=NULL;
	if (url.Scheme()=="http"||url.Scheme()=="https"||url.Scheme()=="ftp") {
		//cerr << "HInt:"<<hints["content-type"]<<endl;
		if (url.Extension()=="torrent" || (hints["content-type"]=="application/x-bittorrent")) {
			dl=this->NewTorrentDownloader();
		}
	}
	return dl;
}

void TorrentDownloadManager::SetDownloadThrottle(int throttle){

	this->s.set_download_rate_limit(throttle);

	try{
		FtdConfig& cfg=FtdConfig::Instance();
		cfg.SetInteger("torrent","maxdownload",throttle);
	}catch(std::runtime_error& err){
		syslog(LOG_INFO,"Unable to read max download speed from config using defaults");
	}
}

int TorrentDownloadManager::GetDownloadThrottle(void){

	if(this->isRunning){
		int ret=this->s.download_rate_limit();

// Work around "bug" in library
#if LIBTORRENT_MAJOR == 0 && LIBTORRENT_VERSION_MINOR == 12
		if(ret==2147483647){
			ret=-1;
		}
#endif
		return ret;
	}else{
		// We havnt started yet and hence have no valid throttle read from cfg
		int ret=0;
		try{
			FtdConfig& cfg=FtdConfig::Instance();
			ret=cfg.GetInteger("torrent","maxdownload");
		}catch(std::runtime_error& err){
			syslog(LOG_INFO,"Unable to read max download speed from config using defaults");
		}
		return ret;
	}
}

void TorrentDownloadManager::SetUploadThrottle(int throttle){

	this->s.set_upload_rate_limit(throttle);

	try{
		FtdConfig& cfg=FtdConfig::Instance();
		cfg.SetInteger("torrent","maxupload",throttle);
	}catch(std::runtime_error& err){
		syslog(LOG_INFO,"Unable to read max upload speed from config using defaults");
	}
}

int TorrentDownloadManager::GetUploadThrottle(void){

	if(this->isRunning){
		int ret=this->s.upload_rate_limit();
// Work around "bug" in library
#if LIBTORRENT_MAJOR == 0 && LIBTORRENT_VERSION_MINOR == 12
		if(ret==2147483647){
			ret=-1;
		}
#endif
		return ret;
	}else{
		// We havnt started yet and hence have no valid throttle read from cfg
		int ret=0;
		try{
			FtdConfig& cfg=FtdConfig::Instance();
			ret=cfg.GetInteger("torrent","maxupload");
		}catch(std::runtime_error& err){
			syslog(LOG_INFO,"Unable to read max upload speed from config using defaults");
		}
		return ret;
	}
}



bool TorrentDownloadManager::ProvidesService(const string& service){
	return service=="torrent";
}

void TorrentDownloadManager::Shutdown(){
	this->isRunning=false;
	DirWatcher::Instance().Stop();
	FtdConfig& cfg=FtdConfig::Instance();

	if(cfg.GetBoolOrDefault("torrent","dhtsupport",false)){
		string stpath=cfg.GetStringOrDefault("general","statedir","/etc/ftd");
		if(Stat::DirExists(stpath)){
			entry e;
			this->s.save_state(e);
			vector<char> buffer;
			bencode(std::back_inserter(buffer),e);
			std::fstream of(string(stpath+"/dhtstate").c_str(),std::fstream::out|std::fstream::binary);
			std::ostream_iterator<char> oI(of);
			copy(buffer.begin(),buffer.end(),oI);
			of.close();
		}
	}
}

