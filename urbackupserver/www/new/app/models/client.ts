import {Process} from "./process";

export class Client {
	id: number;
	name: string;
	showDetail: boolean;
	fileOk: boolean;
	imageOk: boolean;
	lastFileBackup: Date;
	lastImageBackup: Date;
	lastSeen: Date;
	online: boolean;
	version: string;
	osVersion: string;
	selected: boolean;
	status: ClientStatus;
	processes: Process[];
	ip: string;
  
	constructor(name : string) {
        this.name = name;
		this.selected = false;
		this.showDetail = false;
		this.status = ClientStatus.Ok;
		this.processes = [];
	}

	isCreatingBackups() {
		return this.processes.length > 0;
	}

	getTotalProcessPercentage() {
		var totalPercentange = 0;

		for (var i in this.processes) {
			totalPercentange += this.processes[i].percentage;
		}

		return this.processes.length === 0 ? totalPercentange : totalPercentange / this.processes.length;
	}
}