#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include "qdecoder.h"
#include "./hiredis.h"
//#include "./async.h"
//#include "./adapters/libevent.h"

//sync

char *retStr; // made global for the redis callback
char *broker; // made global for the redis callback


/*void getCallBack(redisAsyncContext *c, void *r, void *privdata) {
    redisReply *reply = r;
    if (reply == NULL) return;
    printf("argv[%s]: %s\n", (char*)privdata, reply->str);
    strcat(retStr, "<Accept OrderRefId=\"");
    strcat(retStr, reply->str); //TODO: verify it works
    strcat(retStr, "\" />");
    strcat(retStr, "</Exchange>\n</Response>\n");
    printf("%s\n",retStr);
    //TODO: send via curl
    redisAsyncDisconnect(c);
    printf("%s\n",retStr);
}
*/

int main(void)
{
    retStr = malloc(sizeof(char)*400); // made global for the redis callback
    broker = malloc(sizeof(char)*1000); // made global for the redis callback
    printf("Content-Type: text/plain \n\n");
    //Q_ENTRY *req = qCgiRequestParse(NULL);
    Q_ENTRY *req = qCgiRequestParse(NULL,0 );
    const char *MessageType = req->getStr(req,"MessageType",false);
    const char *From = req->getStr(req,"From",false);
    const char *BS = req->getStr(req,"BS",false);
    const int Shares = req->getInt(req,"Shares");
    const char *Stock = req->getStr(req,"Stock",false);
    const int Price = req->getInt(req,"Price");
    const char *Twilio = req->getStr(req,"Twilio",false);
    const char *BrokerAddr = req->getStr(req,"BrokerAddress",false);
    const int BrokerPort = req->getInt(req,"BrokerPort");
    const char *BrokerEndPoint = req->getStr(req,"BrokerEndpoint",false);
    free(req);
    /*printf("MessageType = %s\n",MessageType);
    printf("From = %s\n",From);
    printf("BS = %s\n",BS);
    printf("Shares = %d\n",Shares);
    printf("Stock = %s\n",Stock);
    printf("Price = %d\n",Price);
    printf("Twilio = %s\n",Twilio);
    printf("BrokerAddress = %s\n",BrokerAddr);
    printf("BrokerPort = %d\n",BrokerPort);
    printf("BrokerEndPoint = %s\n",BrokerEndPoint);
*/
    strcat(retStr, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<Response>\n<Exchange>");
//VALIDATION
    int failure = 0;
    char * failMsg = malloc(sizeof(char)*25);
    if ( MessageType == NULL)
	 { failure = 1; failMsg = "<Reject Reason=\"M\" />";}  
    else if (From == NULL )
	 { failure = 1; failMsg = "<Reject Reason=\"F\" />";}  
    else if (BS==NULL) 
	 { failure = 1; failMsg = "<Reject Reason=\"I\" />";}  
    else if (Stock==NULL) 
	 { failure = 1; failMsg = "<Reject Reason=\"S\" />";}  //FIXME: missing alphanumerical validation...
    else if (Twilio==NULL ) 
	 { failure = 1; failMsg = "<Reject Reason=\"T\" />";}  
    else if (BrokerAddr ==NULL) 
	 { failure = 1; failMsg = "<Reject Reason=\"A\" />";}  
    else if (BrokerEndPoint ==NULL) 
	 { failure = 1; failMsg = "<Reject Reason=\"E\" />";}  

    else if ( strcmp(MessageType, "O") != 0)
	 { failure = 1; failMsg = "<Reject Reason=\"M\" />";}  
    else if (From[0] != '+' || strlen(From) > 16 || strlen(From) < 11 )
	 { failure = 1; failMsg = "<Reject Reason=\"F\" />";}  
    else if (strcmp(BS, "B") != 0 && strcmp(BS,"S")!=0) 
	 { failure = 1; failMsg = "<Reject Reason=\"I\" />";}  
    else if (Shares < 1 || Shares >= 1000000) 
	 { failure = 1; failMsg = "<Reject Reason=\"Z\" />";}  
    else if (Price < 1 || Price >= 100000) 
	 { failure = 1; failMsg = "<Reject Reason=\"X\" />";}  
    else if (strlen(Stock) < 3 || strlen(Stock) > 8) 
	 { failure = 1; failMsg = "<Reject Reason=\"S\" />";}  //FIXME: missing alphanumerical validation...
    else if (strcmp(Twilio, "Y") != 0 && strcmp(Twilio ,"N") !=0 ) 
	 { failure = 1; failMsg = "<Reject Reason=\"T\" />";}  
    else if (BrokerPort < 10 || BrokerPort > 99999) 
	 { failure = 1; failMsg = "<Reject Reason=\"P\" />";}  

    else
    {
	//Verfify phone
	int phone_len = strlen(From);
	int ii;
	for (ii = 1 ; ii < phone_len;ii++)
	{
	    if ( From[ii] < 48 || From[ii] > 57) //assers it is an integer
		{ failure = 1; failMsg = "<Reject Reason=\"F\" />"; goto end_verification;}  
	}
	//verify stock
	int strLen = strlen(Stock);
	for (ii = 0 ; ii < strLen;ii++)
	{
	    if ( (Stock[ii] < 48 || Stock[ii] > 57) 
		&& (Stock[ii] < 65 || Stock[ii] > 90)
		&& (Stock[ii] < 97 || Stock[ii] > 122)) 
		{ failure = 1; failMsg = "<Reject Reason=\"S\" />"; goto end_verification;}  
	}
	//
	strLen = strlen(BrokerAddr);
	if (strLen < 4)
		{ failure = 1; failMsg = "<Reject Reason=\"A\" />"; goto end_verification;}  
	for (ii = 0 ; ii < strLen;ii++)
	{
	    if ( (BrokerAddr[ii] < 48 || BrokerAddr[ii] > 57) 
		&& (BrokerAddr[ii] != 46) && (BrokerAddr[ii] != 45) && (BrokerAddr[ii] != 95)
		&& (BrokerAddr[ii] < 65 || BrokerAddr[ii] > 90)
		&& (BrokerAddr[ii] < 97 || BrokerAddr[ii] > 122)) 
		{ failure = 1; failMsg = "<Reject Reason=\"A\" />"; goto end_verification;}  
	}
	
	//verify BrokerEndpoint
	if (strlen(BrokerEndPoint) < 1) 
	    { failure = 1; failMsg = "<Reject Reason=\"E\" />"; goto end_verification;}  

    }
    
end_verification:
    //TODO: phone verification
    //
    if ( failure == 1) {
	strcat(retStr,failMsg);
	strcat(retStr, "</Exchange>\n</Response>\n");
	printf("%s\n",retStr);
    }
    else 
    {
	//construct broker string:
	char * buf = malloc (sizeof(char)*7);
	sprintf(buf, "%d",BrokerPort);
	
	strcat(broker,BrokerAddr);
	strcat(broker,":");
	strcat(broker,buf);
	strcat(broker,BrokerEndPoint);
	free(buf);
        //redisAsyncContext *c = redisAsyncConnect("127.0.0.1", 6379); //FIXME: 
	redisContext *c;
	redisReply *reply;

	struct timeval timeout = { 1, 500000 }; // 1.5 seconds
        c = redisConnectWithTimeout((char*)"127.0.0.2", 6379, timeout);

	if (c->err) {
            exit(1);
	}
	if ( strcmp(BS,"B")==0) BS = "b";
	else if ( strcmp(BS, "S")==0) BS = "s";
	//ALL THE LOGIC!!!!
	//02fbe601b34a60b554fc0444157de4815a922442
	//
	char * shares_buf = malloc (sizeof(char)*10);
	sprintf(shares_buf, "%d",Shares);
	char * price_buf =  malloc(sizeof(char)*10);
	sprintf(price_buf, "%d",Price);
	time_t times = time(NULL);
	struct tm * mytime =localtime(&times);
	struct timeval tv;
	gettimeofday (&tv, NULL);
	char * timestamp = malloc(sizeof(char)*200);
	sprintf(timestamp, "%d-%.2d-%.2dT%.2d:%.2d:%.2d.%ld", 
		mytime->tm_year+1900,
		mytime->tm_mon+1,
		mytime->tm_mday,
		mytime->tm_hour,
		mytime->tm_min,
		mytime->tm_sec,
		(tv.tv_usec/100000)
		);
	//redisAsyncCommand(c, getCallBack, NULL,
	reply = redisCommand(c, 
	    "evalsha 02fbe601b34a60b554fc0444157de4815a922442 2 %s %s %s %s %s %s %s %s",
	    Stock, 
	    BS,
	    From,
	    shares_buf,
	    price_buf,
	    Twilio,
	    broker,
	    timestamp
	    );
	free(price_buf);
	free(shares_buf);
	free(timestamp);
	if (reply == NULL) return 0;
	strcat(retStr, "<Accept OrderRefId=\"");
        strcat(retStr, reply->str); //TODO: verify it works
	strcat(retStr, "\" />");
        strcat(retStr, "</Exchange>\n</Response>\n");
	printf("%s\n",retStr);
	//redisAsyncDisconnect(c);
	redisFree(c);
        freeReplyObject(reply);

    }
    
    return 0;
}
