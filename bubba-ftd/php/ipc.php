<?
/* Command status */
define("CMD_OK",0);
define("CMD_FAIL",1);
define("ADD_DOWNLOAD",2);
define("CANCEL_DOWNLOAD",3);
define("LIST_DOWNLOADS",4);
define("GET_UPLOAD_THROTTLE",5);
define("SET_UPLOAD_THROTTLE",6);
define("GET_DOWNLOAD_THROTTLE",7);
define("SET_DOWNLOAD_THROTTLE",8);
define("CMD_SHUTDOWN",5);

/* Download status */
define("NOTSTARTED",0);
define("QUEUED",1);
define("DOWNLOADED",2);
define("CANCELINPROGRESS",3);
define("CANCELDONE",4);
define("INPROGRESS",5);
define("FAILED",6);

/* Policy values */
define("DLP_NONE",0x0000);
define("DLP_AUTOREMOVE",0x0001);
define("DLP_HIDDEN",0x0002);

class Downloader{

	var $fp;

	function _connect(){

		$this->fp = @stream_socket_client("unix:///tmp/ftdaemon", $errno, $errstr, 5);
		return !($this->fp===FALSE);
	}
            
	function add_download($user, $url, $uuid="",$policy=DLP_NONE){
		if(!$this->_connect()){
			return false;
		}
		$req=array();
		$req["cmd"]=ADD_DOWNLOAD;
		$req["user"]=$user;
		$req["url"]=$url;
		$req["uuid"]=$uuid==""?uniqid("ftdp"):$uuid;
		$req["policy"]=$policy;
		
		fwrite($this->fp,json_encode($req));
		$res=json_decode(fgets($this->fp,16384),true);
		
		if($res["cmd"]==CMD_FAIL){
			return false;
		}else if($res["cmd"]==CMD_OK){
			return true;
		}else{
			print "Unknown reply<br>";
			return false;
		}
	}

	function list_unordered_downloads($user){
		if(!$this->_connect()){
			return NULL;
		}
		$req=array();
		$req["cmd"]=LIST_DOWNLOADS;
		$req["user"]=$user;
		$req["uuid"]="";
		fwrite($this->fp,json_encode($req));

		$res=json_decode(fgets($this->fp,16384),true);
		if($res["cmd"]==LIST_DOWNLOADS){

			do{
				$ret[]=$res;
				$res=json_decode(fgets($this->fp,16384),true);
			}while($res["cmd"]==LIST_DOWNLOADS);

		}else{
			return NULL;
		}

		return $ret;

	}


	function list_downloads($user){
		if(!$this->_connect()){
			return NULL;
		}
		$req=array();
		$req["cmd"]=LIST_DOWNLOADS;
		$req["user"]=$user;
		$req["uuid"]="";
		fwrite($this->fp,json_encode($req));

		$res=json_decode(fgets($this->fp,16384),true);
		if($res["cmd"]==LIST_DOWNLOADS){

			do{
				$ret[$res['status']][]=$res;
				$res=json_decode(fgets($this->fp,16384),true);
			}while($res["cmd"]==LIST_DOWNLOADS);

		}else{
			return NULL;
		}

		return $ret;

	}

	function querybyUUID($user, $uuid){
		if(!$this->_connect()){
			return NULL;
		}
		$req=array();
		$req["cmd"]=LIST_DOWNLOADS;
		$req["user"]=$user;
		$req["uuid"]=$uuid;
		fwrite($this->fp,json_encode($req));

		$res=json_decode(fgets($this->fp,16384),true);
		if($res["cmd"]==LIST_DOWNLOADS){
			do{
				$ret=$res;
				$res=json_decode(fgets($this->fp,16384),true);
			}while($res["cmd"]==LIST_DOWNLOADS);

		}else{
			return NULL;
		}

		return $ret;
	}


	function cancel_download($user,$url,$uuid=""){
		if(!$this->_connect()){
			return false;
		}
		$req=array();
		$req["cmd"]=CANCEL_DOWNLOAD;
		$req["user"]=$user;
		$req["uuid"]=$uuid;
		$req["url"]=$url;
		fwrite($this->fp,json_encode($req));

		$res=json_decode(fgets($this->fp,16384),true);
		if($res["cmd"]==CMD_FAIL){
			return false;
		}else if($res["cmd"]==CMD_OK){
			return true;
		}else{
			print "Unknown reply<br>";
			return false;
		}
	}
            
	function get_download_throttle($service){
		if(!$this->_connect()){
			return NULL;
		}
		$req=array();
		$req["cmd"]=GET_DOWNLOAD_THROTTLE;
		$req["name"]=$service;
		fwrite($this->fp,json_encode($req));

		$res=json_decode(fgets($this->fp,16384),true);
		if($res["cmd"]==GET_DOWNLOAD_THROTTLE){
			return $res["size"]<0?-1:$res["size"];
		}else{
			return NULL;
		}
	}

	function set_download_throttle($service,$throttle){
		if(!$this->_connect()){
			return false;
		}
		$req=array();
		$req["cmd"]=SET_DOWNLOAD_THROTTLE;
		$req["name"]=$service;
		$req["size"]=$throttle;
		fwrite($this->fp,json_encode($req));

		$res=json_decode(fgets($this->fp,16384),true);
		if($res["cmd"]==CMD_OK){
			return true;
		}else{
			return false;
		}
	}

	function get_upload_throttle($service){
		if(!$this->_connect()){
			return NULL;
		}
		$req=array();
		$req["cmd"]=GET_UPLOAD_THROTTLE;
		$req["name"]=$service;
		fwrite($this->fp,json_encode($req));

		$res=json_decode(fgets($this->fp,16384),true);
		if($res["cmd"]==GET_UPLOAD_THROTTLE){
			return $res["size"]<0?-1:$res["size"];
		}else{
			return NULL;
		}
	}

	function set_upload_throttle($service,$throttle){
		if(!$this->_connect()){
			return false;
		}
		$req=array();
		$req["cmd"]=SET_UPLOAD_THROTTLE;
		$req["name"]=$service;
		$req["size"]=$throttle;
		fwrite($this->fp,json_encode($req));

		$res=json_decode(fgets($this->fp,16384),true);
		if($res["cmd"]==CMD_OK){
			return true;
		}else{
			return false;
		}
	}
}
            
?>
