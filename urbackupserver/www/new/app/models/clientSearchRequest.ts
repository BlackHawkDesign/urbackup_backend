export class ClientSearchRequest {
	name: string;
	online: number;
	status: number;
	pageNumber: number;
	pageSize: number;
  
	constructor() {
		this.name = "";
		this.online = -1;
		this.status = -1;
		this.pageNumber = 1;
		this.pageSize = 10;
	}
}