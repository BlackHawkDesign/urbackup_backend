export class ClientFilterRequest {
  name: string;
  online: string;
  status: string;
  
  constructor() {
		this.name = "";
		this.online = "";
		this.status = "";
	}
}