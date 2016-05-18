import { Directive, ElementRef, Input } from '@angular/core';

@Directive({ selector: '[numberBinder]' })

export class NumberBinderDirective {
	constructor(el: any) {
		
	}

	link(scope, element, attrs, ngModel) {
		/*
		ngModel.$parsers.push(function (val) {
			return val ? parseInt(val, 10) : null;
		});
		ngModel.$formatters.push(function (val) {
			return val ? '' + val : null;
		});*/
		alert("test!");
	}
}