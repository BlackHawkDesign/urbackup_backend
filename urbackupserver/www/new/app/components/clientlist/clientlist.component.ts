import {Component} from 'angular2/core';
import {ClientSearchRequest} from './../../models/clientSearchRequest';
import {ClientSearchResult} from './../../models/clientSearchResult';
import {ClientService} from './../../services/client.service';

@Component({
    selector: 'clientlist',
	templateUrl: './app/components/clientlist/clientlist.html',
	providers:[ClientService]
})

export class ClientListComponent {  
	clientService : ClientService;
	searchRequest : ClientSearchRequest;
	searchResult : ClientSearchResult;
	
	constructor(clientService: ClientService) {
		this.clientService = clientService;
		this.searchRequest = new ClientSearchRequest();
		this.search();
	}
	
	search(){
		this.searchResult = this.clientService.getClients(this.searchRequest);
	}
	
	toggleClientDetail(event: any){
		var row = $(event.currentTarget).parent();
		var detailRow = row.next();
		detailRow.toggle();
	}
	
	toggleClient(event: any, client: any){
		client.selected = event.currentTarget.checked;	
	}
	
	getSelectedClients(){		
		var clients = [];
		
		for (var i in this.searchResult.clients){
			if(this.searchResult.clients[i].selected){
				clients.push(this.searchResult.clients[i]);
			}
		}
		
		return clients;
	}
}