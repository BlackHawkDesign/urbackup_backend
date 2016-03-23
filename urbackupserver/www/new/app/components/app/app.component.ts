import {Component} from 'angular2/core';
import {MenuComponent} from './../menu/menu.component';
import {ClientListComponent} from './../clientlist/clientlist.component';

@Component({
    selector: 'urbackup-app',
	templateUrl: './app/components/app/app.html',
	directives: [MenuComponent,ClientListComponent]
})

export class AppComponent { 

}