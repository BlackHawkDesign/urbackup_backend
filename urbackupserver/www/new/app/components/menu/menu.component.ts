import {Component} from '@angular/core';
import {ROUTER_DIRECTIVES,ROUTER_PROVIDERS} from '@angular/router';

@Component({
    selector: 'menu',
	templateUrl: './app/components/menu/menu.html',
	directives: [ROUTER_DIRECTIVES]
})

export class MenuComponent {  
	
	getActiveClass(path : string){
		return true ? 'active' : '';
	}
}