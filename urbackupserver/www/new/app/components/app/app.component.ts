import {Component} from 'angular2/core';
import {RouteConfig} from 'angular2/router';
import {ROUTER_DIRECTIVES,ROUTER_PROVIDERS} from 'angular2/router';
import {MenuComponent} from './../menu/menu.component';
import {ClientListComponent} from './../clientlist/clientlist.component';
import {LogTableComponent} from './../logtable/logtable.component';
 
@Component({
    selector: 'urbackup-app',
	templateUrl: './app/components/app/app.html',
	directives: [MenuComponent,ClientListComponent,ROUTER_DIRECTIVES]
})

@RouteConfig([
	{path:'/',				name: 'Root',			component: ClientListComponent},
	{path:'/clients',		name: 'Clients',		component: ClientListComponent},
	{path:'/logs',			name: 'Logs',			component: LogTableComponent}
])
export class AppComponent { 

}