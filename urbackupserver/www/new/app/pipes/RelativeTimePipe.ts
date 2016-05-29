///<reference path='../../typings/modules/moment/index.d.ts'/>
import { Pipe, PipeTransform } from '@angular/core';
import {Moment} from 'moment';

@Pipe({
	name: 'relativeTime'
})

export class RelativeTimePipe implements PipeTransform {
	transform(value: Date, exponent: string): string {
		//return moment(value).fromNow();

		return "blabla";
	}
}