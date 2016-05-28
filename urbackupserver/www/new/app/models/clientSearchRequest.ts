export class ClientSearchRequest {
	name: string;
	online: number;
	status: string;
	pageNumber: number;
	pageSize: number;
  
	constructor() {
		this.name = "";
		this.online = -1;
		this.status = "";
		this.pageNumber = 1;
		this.pageSize = 10;
	}
}