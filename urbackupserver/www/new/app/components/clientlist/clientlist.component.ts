import {Component} from 'angular2/core';
import {Client} from './../../models/client';
import {ClientFilterRequest} from './../../models/clientFilterRequest';
import {ClientService} from './../../services/client.service';

@Component({
    selector: 'clientlist',
	templateUrl: './app/components/clientlist/clientlist.html',
	providers:[ClientService]
})

export class ClientListComponent {  
	clientService : ClientService;
	clients : Client[];
	filter : ClientFilterRequest;
	
	constructor(clientService: ClientService) {
		this.clientService = clientService;
		this.filter = new ClientFilterRequest();
		this.executeFilter();
	}
	
	executeFilter(){
		this.clients = this.clientService.getClients(this.filter);
	}
	
	displayDetail(clientNameCell){
		var row = $(clientNameCell).parent();
		var detailRow = row.next();
		detailRow.show();
	}
}