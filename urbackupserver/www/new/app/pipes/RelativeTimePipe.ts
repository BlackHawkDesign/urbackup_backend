import { Pipe, PipeTransform } from '@angular/core';
import * as moment from 'moment/moment';

@Pipe({
	name: 'relativeTime'
})

export class RelativeTimePipe implements PipeTransform {
	transform(value: Date, exponent: string): string {
		return moment(value).fromNow();
	}
}