import {Component} from '@angular/core';
import {Router,Routes, ROUTER_DIRECTIVES, ROUTER_PROVIDERS} from '@angular/router';
import {MenuComponent} from './../menu/menu.component';
import {ClientListComponent} from './../clientlist/clientlist.component';
import {LogTableComponent} from './../logtable/logtable.component';
import {ActivityListComponent} from "../activitylist/activitylist.component";

@Component({
    selector: 'urbackup-app',
	templateUrl: './app/components/app/app.html',
	directives: [MenuComponent,ClientListComponent,ROUTER_DIRECTIVES]
})

@Routes([
	{ path:'/',				component: ClientListComponent},
	{ path:'/clients',		component: ClientListComponent},
	{ path: '/activities',  component: ActivityListComponent },
	{ path:'/logs',			component: LogTableComponent}
])
export class AppComponent { 

}