<?
	define("CMD_OK",0);
	define("CMD_FAIL",1);
	define("ADD_DOWNLOAD",2);
	define("CANCEL_DOWNLOAD",3);
	define("LIST_DOWNLOADS",4);
	define("CMD_SHUTDOWN",5);
	
	class Downloader{

		var $que;

		function Downloader(){
			touch("/tmp/ftdaemon");
			$this->que=msg_get_queue(ftok("/tmp/ftdaemon","A"));
		}
		
		function send_command($user,$cmd,$url){
			$msg=pack(
				"La32Ia256LL",
				getmypid(),
				$user,
				$cmd,
				$url,
				0,0);
				
			if (!msg_send ($this->que, 1, $msg, false, true, $msg_err))
				return false;
				
			return true;
		}

		function receive_command(){

			if (!msg_receive($this->que,getmypid(),$msg_type,16384,$the_msg,false)){
				return false;
			}else{
				$arr=unpack("Ladress/a32user/Icmdtype/a256url/Lsize/Ldownloaded",$the_msg);
			}
			return $arr;
		
		}

		function add_download($user, $url){
			$this->send_command($user,ADD_DOWNLOAD,$url);
			$res=$this->receive_command();
			if($res["cmdtype"]==CMD_FAIL){
				return false;
			}else if($res["cmdtype"]==CMD_OK){
				return true;
			}else{
					print "Unknown reply<br>";
				return false;
			}
		}
		
		function list_downloads($user){
			$this->send_command($user,LIST_DOWNLOADS,"");

			$res=$this->receive_command();

			if($res["cmdtype"]==LIST_DOWNLOADS){
				
				do{
					$ret[]=$res;
					$res=$this->receive_command();
				}while($res["cmdtype"]==LIST_DOWNLOADS);
				
			}else{
				return NULL;
			}

			return $ret;
			
		}

		function cancel_download($user,$url){
			$this->send_command($user,CANCEL_DOWNLOAD,$url);
			$res=$this->receive_command();
			if($res["cmdtype"]==CMD_FAIL){
					return false;
				}else if($res["cmdtype"]==CMD_OK){
				return true;
			}else{
				print "Unknown reply<br>";
				return false;
			}
		}

		
	}
	
	function show_downloads($dls){
		if($dls!=NULL){
			foreach($dls as $key => $val){
				print "\n\tUser: ".$val["user"]."\n";
				print "\t Url: ".$val["url"]."\n";
				if($val["size"]==0){
					$dld=0;
				}else{
					$dld=($val["downloaded"]/$val["size"])*100;
				}
				print "\tDone: ".$dld."%\n";
				print "\tSize: ".$val["size"]."\n";
			}
		}
				
	}
	
	$dl=new Downloader;

	if($argc>1){
		if($argv[1]=="cancel" && $argc==4){
			$dl->cancel_download($argv[2],$argv[3]);
		}else{
			$dl->add_download("Pelle Testare",$argv[1]);
		}
	}
	
	$dls=$dl->list_downloads("*");
	
	var_dump($dls);

	show_downloads($dls);


	
?>
