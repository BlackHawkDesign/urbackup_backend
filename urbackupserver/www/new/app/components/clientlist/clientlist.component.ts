import {Component} from 'angular2/core';
import {Client} from './../../models/client';
import {ClientSearchRequest} from './../../models/clientSearchRequest';
import {ClientService} from './../../services/client.service';

@Component({
    selector: 'clientlist',
	templateUrl: './app/components/clientlist/clientlist.html',
	providers:[ClientService]
})

export class ClientListComponent {  
	clientService : ClientService;
	clients : Client[];
	searchRequest : ClientSearchRequest;
	
	constructor(clientService: ClientService) {
		this.clientService = clientService;
		this.searchRequest = new ClientSearchRequest();
		this.executeFilter();
	}
	
	executeFilter(){
		this.clients = this.clientService.getClients(this.searchRequest);
	}
	
	displayDetail(event: any){
		var row = $(event.currentTarget).parent();
		var detailRow = row.next();
		detailRow.toggle();
	}
}