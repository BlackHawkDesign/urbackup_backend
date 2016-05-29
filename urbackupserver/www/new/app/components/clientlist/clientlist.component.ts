import {Component} from '@angular/core';
import {ClientSearchRequest} from './../../models/clientSearchRequest';
import {ClientSearchResult} from './../../models/clientSearchResult';
import {Client} from './../../models/client';
import {ClientService} from './../../services/client.service';
import {RelativeTimePipe} from './../../pipes/relativeTimePipe';

@Component({
    selector: 'clientlist',
	templateUrl: './app/components/clientlist/clientlist.html',
	providers: [ClientService],
	pipes: [RelativeTimePipe],
})

export class ClientListComponent {  
	clientService : ClientService;
	searchRequest : ClientSearchRequest;
	searchResult: ClientSearchResult;
	
	constructor(clientService: ClientService) {
		this.clientService = clientService;
		this.searchRequest = new ClientSearchRequest();
		this.search();
	}
	
	search(pageNumber: number = null) {
		if (pageNumber != null) {
			this.searchRequest.pageNumber = pageNumber;
		}

		this.searchResult = this.clientService.getClients(this.searchRequest);
	}

	toggleClientSelection(event: any, client: any) {
		client.selected = event.currentTarget.checked;
	}
	
	toggleClientDetail(client: Client) {
		client.showDetail = !client.showDetail;
	}
	
	toggleAllClientsSelection(event: any){
		for (var i in this.searchResult.clients){
			this.searchResult.clients[i].selected = event.currentTarget.checked;	
		}
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