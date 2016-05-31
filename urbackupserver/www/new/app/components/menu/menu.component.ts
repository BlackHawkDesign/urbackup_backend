import {Component} from '@angular/core';
import {Router, ROUTER_DIRECTIVES,ROUTER_PROVIDERS} from '@angular/router';

@Component({
    selector: 'menu',
	templateUrl: './app/components/menu/menu.html',
	directives: [ROUTER_DIRECTIVES]
})

export class MenuComponent {  
	router: Router;

	constructor(router: Router) {
		this.router = router;
	}

	isActive(path : any, iets : any) {


		return true;
	}
}