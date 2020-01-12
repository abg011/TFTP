/* attempt 1 - UDP select() with timeout */
#ifndef UDP_SELECT_TIMEOUT
#define UDP_SELECT_TIMEOUT

struct rtt_info {
	//our basic structure to store rtt related stuff
	float rtt_rtt; //most recent RTT
	float rtt_srtt; //smoothed RTT estimator
	float rtt_rttvar; //smoothed mean deviation
	float rtt_rto; //current RTO to use
	int rtt_nrexmt; //times retransmitted
	uint32_t rtt_base; //time since UNIX EPOCH
};

#define RTT_RXTMIN 2 //min timeout time, (s)
#define RTT_RXTMAX 30
#define RTT_MAXNREXMT 3 //max time to retransmit is 3 (as per ques)

/* function declarations */
void rtt_debug(struct rtt_info*);
void rrt_init(struct rtt_info*);
void rtt_newpack(struct rtt_info*);
int rtt_start(struct rtt_info*);
void rtt_stop(struct rtt_info*, uint32_t);
int rtt_timeout(struct rtt_info*);
uint32_t rtt_ts(struct rtt_info*);

int rtt_d_flag = 0;

/*
calculate RTO value based on current estimators: srtt and rttvar
*/
#define RTT_RTOCALC(ptr) ((ptr)->rtt_srtt + (4.0 * (ptr)->rtt_rttvar))

static float rtt_minmax(float rto) {
	if(rto < RTT_RXTMIN)
		rto = RTT_RXTMIN;
	else if(rto > RTT_RXTMAX)
		rto = RTT_RXTMAX;
	return (rto);
}

/*
called first time any packet is sent from the client
*/
void rtt_init(struct rtt_info* ptr) {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	ptr->rtt_base = tv.tv_sec;
	ptr->rtt_rtt = 0;
	ptr->rtt_srtt = 0;
	ptr->rtt_rttvar = 0.75;
	ptr->rtt_rto = rtt_minmax(RTT_RTOCALC(ptr));
	fprintf(stdout, "Most recent RTT: %f, Current RTO:  %f\n", ptr->rtt_rtt, ptr->rtt_rto);
}

/*
returns the current timestamp for the caller to store as uint32_t
*/
uint32_t rtt_ts(struct rtt_info* ptr) {
	uint32_t ts;
	struct timeval tv;
	gettimeofday(&tv, NULL);
	ts = ((tv.tv_sec - ptr->rtt_base)*1000) + (tv.tv_usec/1000);
	return (ts);
}

/*
set the retransmission counter to 0 whenever a new packet is sent for first time
*/
void rtt_newpack(struct rtt_info* ptr) {
	ptr->rtt_nrexmt = 0;
}

//return the current RTO in seconds. can be used as alarm (TODO)
int rtt_start(struct rtt_info* ptr) {
	return ((int)(ptr->rtt_rto + 0.5));
}

/*
called after a reply is received and update the RTT to calculate new RTO
*/
void rtt_stop(struct rtt_info* ptr, uint32_t ms) {
	double delta;
	ptr->rtt_rtt = ms/1000.0;
	delta = ptr->rtt_rtt - ptr->rtt_srtt;
	ptr->rtt_srtt += delta/8; //g = 1/8 idk why lol
	if(delta < 0.0) {
		//take mod duh
		delta -= delta;
	}
	ptr->rtt_rttvar += (delta - ptr->rtt_rttvar)/4;
	ptr->rtt_rto = rtt_minmax(RTT_RTOCALC(ptr));
}

/*
exponential backoff when max number of transmissions is reached
*/
int rtt_timeout(struct rtt_info* ptr) {
	ptr->rtt_rto *= 2;
	if(++ptr->rtt_nrexmt > RTT_MAXNREXMT)
		return -1;
	return 0;
}

#endif