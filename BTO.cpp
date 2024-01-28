#include<iostream>
#include<stdlib.h>
#include<unordered_set>
#include<mutex>
#include<shared_mutex>
#include<time.h>
#include<map>
#include<unistd.h>
#include<thread>
#include<time.h>
#include<math.h>
#include<vector>
#include<fstream>
#include<iostream>

using namespace std ;

class Transaction{
    private:
    char tranSatus ; // 
    int m ; //no of dataitems

    public:
    int transId ;
    int readTrans=-1 ;
    int other=-1 ;
    unordered_set <int> write_set;  // To keep track of varibales
	unordered_set <int> read_set; // To keep track of objects the given trasaction has read from.
    int localBuffer ;

    Transaction(int i , int M){
        transId=i ;
        m=M ;
        tranSatus='s' ;
    }
    void set_status( char c){
        tranSatus=c;
    }
    int get_status(){
        return tranSatus ;
    }
};

class Dataitem{
    public:
	static const int initial =-2;
	int value; // It will benote the value corresponding a particular object of this type
	int index=0;
	int arr[30];
    mutex lock;  // to lock the given object 
    int W_TS=-1;
	int R_TS=-1;
	int TS=-1;
    unordered_set <int> read_list;
	Dataitem()=default;
	Dataitem(int v){
        value=v ;
    }
};

class BTO
{
	private:
	int counter;  // Counting the no of transactions

    public:
	int M;
    vector<Dataitem*> dataItems;  // A set of data items
    map<int,Transaction*> transaction;//// A set of transactions
    shared_mutex scheduler_lock;  // A shared mutex lock to lock the transactions

    // Creating constructors

	BTO(int m): M(m)     // Initializing the constructor
{	
	counter=0;  // The counter is initialized to 0
	dataItems=std::vector<Dataitem*>();  
         
	for(int i=0;i<M;i++) dataItems.push_back(new Dataitem(Dataitem::initial)); // initialinzing
	
	transaction =std::map<int,Transaction*>(); 
	
}  

    // Creating the respective functions.
    
	int begin_trans(){
        scheduler_lock.lock() ;
        int id= counter ;
        transaction[id]= new Transaction(id,M);
        counter++ ;
        scheduler_lock.unlock() ;
        return id;
    }

	int read(int id,int array_value,int local_value){
        scheduler_lock.lock_shared() ;
        dataItems[array_value]->lock.lock();
        Transaction* trans= transaction[id];

        if ( dataItems[array_value]->W_TS>id ){
		trans->set_status('a');  // If some transaction with higher time stamp already written then abort
		dataItems[array_value]->lock.unlock();
		scheduler_lock.unlock_shared();
		return -1;
	    }
	    else{
		trans->localBuffer=dataItems[array_value]->value;  // else allow it to read the value and set new timestamp
		dataItems[array_value]->R_TS= (dataItems[array_value]->R_TS>id)?dataItems[array_value]->R_TS:id;
	    }
	
	    dataItems[array_value]->lock.unlock();
	
	    scheduler_lock.unlock_shared();
	    return local_value;
    }

	int write(int id,int array_value,int local_value ){
        scheduler_lock.lock_shared();
	    Transaction* trans= transaction[id];
	    if(dataItems[array_value]->R_TS>id || dataItems[array_value]->W_TS>id){
	    // If some higher time stamp has read or written then abort
		    trans->set_status('a');
		    scheduler_lock.unlock_shared();
		    return -1;
	    }
	    else{
	        transaction[id]->localBuffer=local_value;  // writing on the local value associated with the trnsaction
	        dataItems[array_value]->W_TS=id;
	        transaction[id]->write_set.insert(array_value);
	    }
	    scheduler_lock.unlock_shared();
	    return 0;
    }
	int trycommit(int id){
		int val ;
        scheduler_lock.lock_shared();
		if(transaction[id]->get_status()=='a'){
			val=-1 ;
		}else{
			val= 1 ;
		}
        for(int i:transaction[id]->write_set){
	        dataItems[i]->value=transaction[id]->localBuffer;
		}
        scheduler_lock.unlock_shared();	
        return val;
    }

	void garbageCollect(){
        for(auto&[k,v]:transaction){
	        if(v->get_status()=='s')   // No need to process ongoing transactions
		    continue;
        delete transaction[k];
        }
    }

	~BTO() {
    garbageCollect();
    for(auto e: dataItems){
        delete(e);
        }
    }
};

int getRand(int k){
	return rand()%k;
}

float getExpRand(int k){
	//return log(1+rand()%k);
	return log(1+rand()%k);
}
namespace g{
	int n =10;  // No of threads to create
	int m=10;   // The array has m shared variables
	int numTrans = 10;  // No of transactions
	int const_val= 10;  
	int lambda = 10;  // input for the getExpRand function 

	std::vector<int> total_aborts;
 	std::vector<uint64_t> total_delay;

	FILE *log_file;
}

void udtMem( BTO* b,int mTid)
{
    char buffer[80];
	int status=-1;  // The status of the transaction
	int abort_count =  0;  // To keep track of the no of aborted transactions


	

	// each thread will invoke a number of transactions

	for(int i=0;i<g::numTrans;i++)
	{
		abort_count=0;           // reset the abort count
		time_t start_time;
		start_time=time(NULL);  // Keep track critical section start time

		do{

			int id = b->begin_trans();
		//	printf("transaction created\n");
			int rand=getRand(g::m); // Gets a random no of iterations to be updated
			int local_value;
			float rand_time;

			for(int i=0;i<rand;i++)
			{
				int rand_index, rand_val;

				rand_index=getRand(g::m); 
				rand_val=getRand(g::const_val); 
				
				b->read(id,rand_index,local_value); // It will read the value at the index rand_index in the local value
		//		printf("value read\n");
				
			auto g1 = std::chrono:: high_resolution_clock::now();
                std::time_t now1 = std::chrono::system_clock::to_time_t(g1);
                std::strftime(buffer, 80, "%H:%M:%S", std::localtime(&now1));

				fprintf(g::log_file,"Thread id : %ld  Transaction :  %d reads from : %d a value of : %d at time:%s\n",pthread_self(),id,rand_index,local_value,buffer);
				local_value+=rand_val;

				// request to write the value into the transaction
				b->write(id,rand_index,local_value);
		//		printf("value written\n");

				auto g2 = std::chrono:: high_resolution_clock::now();
                std::time_t now2 = std::chrono::system_clock::to_time_t(g2);
                std::strftime(buffer, 80, "%H:%M:%S", std::localtime(&now2));
				fprintf(g::log_file,"Thread id : %ld  Transaction :  %d writes to : %d a value of : %d  at time:%s\n",pthread_self(),id,rand_index,local_value,buffer);

				rand_time=getExpRand(g::lambda);
				usleep(rand_time*1000);

			}
			status = b->trycommit(id);  // Try to commit the transaction
		//	printf("Transaction commited\n");
			auto g3 = std::chrono:: high_resolution_clock::now();
            std::time_t now3 = std::chrono::system_clock::to_time_t(g3);
            std::strftime(buffer, 80, "%H:%M:%S", std::localtime(&now3));
			fprintf(g::log_file,"Transaction :  %d try commits with result : %d  at time: %s \n",id,status,buffer);
			if (status==-1)abort_count++;

		
				
		}while(status!=1);
		
		

		time_t end_time=time(NULL);

	g::total_delay[mTid]+=end_time-start_time;
	g::total_aborts[mTid]+=abort_count;
	}
}

int main()
{	
	// Creating a file
	std:: ifstream inFile("inp-params.txt",std::ifstream::in);
	

	// checking if file pointer is null
	if(inFile.fail())
	{
		printf("File not found");
		exit(1);
	}


	// Reading the global parameters from the file
	inFile>>g::n>>g::m>>g::numTrans>>g::const_val>>g::lambda;

	// initializing the glabal vectord for storing the parameters

	for(int i=0;i<g::n;i++)
	{
		g::total_aborts.push_back(0);
		g::total_delay.push_back(0);
	}
	
	

	// Starting the first scheduler
	printf("----------Results for BTO-------------\n");

	BTO* b =new BTO(g::m);
	

	std::thread tid[g::n];

	g::log_file=fopen("BTO_log_file","w");
	fprintf(g::log_file,"----------Results for BTO-------------\n");


	for(int i=0;i<g::n;i++)
		{
		tid[i]=thread(udtMem,b,i);
	
		}
		
	for(int i=0;i<g::n;i++)
	tid[i].join();

	uint64_t res = 0;

       for(auto e: g::total_delay) res+=e;
	   fprintf(g::log_file, "Avg Delay:%f\n",(((double)res)/(g::n*g::numTrans)));
       std::cout<<"Avg Delay:"<<((((double)res)/(g::n*g::numTrans))*1000)<<"ms"<<std::endl;

     res = 0;

     for(auto e: g::total_aborts) res+=e;
     std::cout<<"Total aborts:"<<res<<std::endl;
	 fprintf(g::log_file, "Total Aborts:%ld\n",res) ;
     std::cout<<"Avg Aborts:"<<(((double)res)/(g::n*g::numTrans))<<std::endl;
	 fprintf(g::log_file, "Avg Aborts:%f\n",(((double)res)/(g::n*g::numTrans)));
	
	
	delete b;
	fclose(g::log_file);
	return 0;
}
