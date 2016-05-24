export class ClientSearchRequest {
	name: string;
	online: boolean;
	status: string;
	page: number;
	pageSize: number;
  
	constructor() {
		this.name = "";
		this.online = null;
		this.status = "";
		this.page = 1;
		this.pageSize = 50;
	}
}