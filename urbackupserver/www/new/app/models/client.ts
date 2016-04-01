export class Client {
  id: number;
  name: string;
  fileOk: boolean;
  imageOk: boolean;
  lastFileBackup: Date;
  lastImageBackup: Date;
  lastSeen: Date;
  online: boolean;
  osVersion: string;
  selected: boolean;
  status: number;
  
  
  constructor(name : string) {
        this.name = name;
		this.selected = false;
    }
}

/*

{
         "client_version_string":"",
         "delete_pending":"",
         "file_ok":false,
         "id":1,
         "image_ok":false,
         "ip":"-",
         "lastbackup":"",
         "lastbackup_image":"",
         "lastseen":"2016-03-31 20:35",
         "name":"test1",
         "online":false,
         "os_version_string":"",
         "processes":[

         ],
         "status":0
      },
	  */