export class ClientSearchRequest {
	name: string;
	online: string;
	status: string;
	page: number;
	pageSize: number;
  
	constructor() {
		this.name = "";
		this.online = "";
		this.status = "";
		this.page = 1;
		this.pageSize = 50;
	}
}