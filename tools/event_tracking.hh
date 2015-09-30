#include "pdsdata/xtc/Dgram.hh"

class event_tracking 
	{
	public:
	event_tracking(int fd, int initial) :
          fulfilled_number(0),
	  request_number(initial),
          pfd_socket(fd)
        {
		if (send(pfd_socket , (char*)&request_number , strlen((char*)&request_number) , 0) < 0) {
		perror("SEND TO REQUEST NUMBER: ");
		printf("REQUEST NUMBER TCP SOCKET JUST MADE: %i\n", request_number);
		}
        }
	private:
	int fulfilled_number;
	int request_number;
	int pfd_socket;

	public:
	void send_request() {

 		if(pfd_socket != 0 ) { 

		request_number= request_number +1; //starts counting after initial buffers filled

		if (send(pfd_socket, (char*)&request_number , sizeof(request_number) , 0) < 0) {
		perror("SEND TO REQUEST NUMBER: ");
		} 
		printf("REQUEST_NUMBER: %i\n", request_number);
		}


	}
	
	char* receive_datagram() {

				Pds::Dgram datagram;
                         int nb = recv(pfd_socket, &datagram, sizeof(datagram), 0) ;
		
				fulfilled_number= fulfilled_number + 1;
				printf("FULFILLED NUMBER: %i\n", fulfilled_number);

                         if (nb==0) return 0;
                         	if  (nb == -1) {
				perror("recv");
				}
        		 printf("%i [%d]\n", nb, datagram.xtc.sizeofPayload());

         		char* p = new char[sizeof(datagram)+datagram.xtc.sizeofPayload()];
         		memcpy(p, &datagram, sizeof(datagram));
	 		char* q = p+sizeof(datagram);
         		int remaining = datagram.xtc.sizeofPayload();
         			do {
           			nb = recv(pfd_socket, q, remaining, 0);
           			remaining -= nb;
           			q += nb;
         			} while (remaining>0);
			return p;			

	}

        int number_pending() {

		int pend_number = request_number - fulfilled_number;
		return pend_number;
	}

	}; 
