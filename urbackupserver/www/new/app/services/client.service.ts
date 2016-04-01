import {Client}   from './../models/client';
import {ClientSearchRequest} from './../models/clientSearchRequest';
import {ClientSearchResult} from './../models/clientSearchResult';

export class ClientService {
	clients : Client[];
	
	/*
	id: number;
  name: string;
  fileOk: boolean;
  imageOk: boolean;
  lastFileBackup: Date;
  lastImageBackup: Date;
  lastSeen: Date;
  Online: boolean;
  OsVersion: string;
  selected: boolean;
  Status: number;*/
	
	constructor() {
		var client = new Client("Pc1");
		client.id = 1;
		client.fileOk = true;
		client.imageOk = true;
		client.lastFileBackup = new Date("2016-03-01");
		client.lastFileBackup = new Date("2016-03-02");
		client.lastSeen = new Date();
		client.online = true;
		client.osVersion = "Windows 7 64 biit";
		client.status = 0;
		
		var client2 = new Client("Pc1");
		client2.id = 1;
		client2.fileOk = true;
		client2.imageOk = true;
		client2.lastFileBackup = new Date("2016-03-01");
		client2.lastFileBackup = new Date("2016-03-02");
		client2.lastSeen = new Date();
		client2.online = false;
		client2.osVersion = "Windows 7 64 biit";
		client2.status = 0;
		
		this.clients = [
			client,
			client2
		];
	}
	
	getClients(searchRequest: ClientSearchRequest) { 
		var clients = [];
		
		for (var i in this.clients){
			if(this.clients[i].name.toUpperCase().search(searchRequest.name.toUpperCase()) != -1){
				clients.push(this.clients[i]);
			}
		}
		
		return new ClientSearchResult(searchRequest, clients); 
	}
}