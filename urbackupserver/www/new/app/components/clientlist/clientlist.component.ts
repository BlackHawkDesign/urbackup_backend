import {Component} from 'angular2/core';

@Component({
    selector: 'clientlist',
	templateUrl: './app/components/clientlist/clientlist.html'
})

export class ClientListComponent {  
	clients = [{"name":"Pc1"},{"name":"PC2"}];
	
	filter = {
		name : "",
		online : "",
		status : ""
	};
	
	executefilter(){
		filteredClients = [];
		
		for (client in clients){
			if(client.name.search(filter.name)){
				filteredClients.push(client);
			}
		}
		
		clients = filteredClients;
	}
}