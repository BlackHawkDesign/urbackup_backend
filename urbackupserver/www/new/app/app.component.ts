import {Component} from 'angular2/core';
import {Menu} from './components/menu/menu';

@Component({
    selector: 'urbackup-app',
	templateUrl: './app/app.html',
	directives: [Menu]
})

export class AppComponent { }