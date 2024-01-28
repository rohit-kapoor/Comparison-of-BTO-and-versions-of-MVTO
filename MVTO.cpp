#include<set>
#include <unordered_set>
#include<iostream>
#include<mutex>
#include<unistd.h>
#include<vector>
#include<map>
#include<stdlib.h>
#include<random>
#include<thread>
#include<fstream>
#include<shared_mutex>

using namespace std ;

namespace g
{
	int n =10;  // No of threads to create
	int m=10;   // The array has m shared variables
	int numTrans = 50;  // No of transactions
	int const_val= 100;  
	int lambda = 20;  // input for the getExpRand function 

	std::vector<int> total_aborts;
 	std::vector<uint64_t> total_delay;

	FILE *log_file;
}


int getRand(int k)
{
	return rand()%k;
}

float getExpRand(int k)
{
	return log(1+rand()%k);
}


class Transaction{
private:
	char tranStatus;  // To descibe the status of the given transaction
	int m;

public:
	int tranId;
	int readTrans=-1;
	int other=-1;
	unordered_set <int> write_set;  // To keep track of varibales
	unordered_set <int> read_set; // To keep track of objects the given trasaction has read from.
	int localBuffer; // For storing the value till the val write phase

	Transaction(int i,int M){
        tranId=i;
        tranStatus='s';
        m=M;
    }

	void set_status( char c){
        tranStatus=c ;
    }

	char get_status(){
        return tranStatus ;
    }
};

class Dataitem{
public:
	static const int initial =-2;
	int value; // It will benote the value corresponding a particular object of this type
	int index=0;
	int arr[30];
	Dataitem()=default;
	Dataitem(int v){
        value=v ;
    }
	unordered_set <int> read_list;
	int w_ts=-1;
	int r_ts=-1;
	int largest_id=-1;
	std::mutex lock;  // to lock the given object 
};

class MVTO{
    private:
        int counter;

    public:

    int M;
    MVTO(int m):M(m){
    counter=0;  // setting the counter initialy to 0 
	dataItems=std::vector<Dataitem*>();           
	for(int i=0;i<M;i++) dataItems.push_back(new Dataitem(Dataitem::initial)); // initialinzing
	transaction =std::map<int,Transaction*>(); 
    }
    shared_mutex scheduler_lock;

    vector<Dataitem*> dataItems;  // A set of data items
    map<int,Transaction*> transaction;  // A set of transactions 

// creating a set of functions...

	int begin_trans(){
        scheduler_lock.lock();  // locking the transaction
	    int id = counter;  // setting the transaction id to counter value
	    transaction[id]= new Transaction(id,M); // creating new object for transaction
	    counter++;
	    scheduler_lock.unlock();

	    return id;    // Considering the id as the time stamp for simplicity
    }

	int read(int id,int array_value,int local_value){
        scheduler_lock.lock_shared(); // applying a shared lock to the transaction
        Transaction* trans= transaction[id]; // creating a pointer to the transaction
        if(dataItems[array_value]->largest_id!=-1){
	        local_value=transaction[dataItems[array_value]->largest_id]->localBuffer; // reading the value corresponding to the previous 
	                                                 // version
	        trans->readTrans=dataItems[array_value]->largest_id;
	        dataItems[array_value]->read_list.insert(id);
	    }
	    local_value=dataItems[array_value]->value;
        dataItems[array_value]->read_list.insert(id);
        scheduler_lock.unlock_shared();
	    return local_value;
    }

	int write(int id,int array_value,int local_value ){
        scheduler_lock.lock_shared();
        Transaction* trans= transaction[id];
        for (int j : dataItems[array_value]->read_list){
	 	if (j>id && transaction[j]->readTrans<id ){
	 		trans->set_status('a');
	 		scheduler_lock.unlock_shared();
	 		return -1;
	 	    }
	    }
	    trans->localBuffer=local_value;
	    scheduler_lock.unlock_shared();
	    return 0;
    }

	int trycommit(int id){
        scheduler_lock.lock_shared();
        if(transaction[id]->get_status()=='a'){
	        scheduler_lock.unlock_shared();
	        return -1;
        }
        for(int i:transaction[id]->write_set)
			dataItems[i]->value=transaction[id]->localBuffer;
        transaction[id]->set_status('c');
        scheduler_lock.unlock_shared();

        return 1;
    }
};

void udtMem( MVTO* mv,int mTid)
{
	int status=-1;  // The status of the transaction
	int abort_count =  0;  // To keep track of the no of aborted transactions
	char buffer[80];


	

	// each thread will invoke a number of transactions

	for(int i=0;i<g::numTrans;i++)
	{
		abort_count=0;           // reset the abort count
		time_t start_time;
		start_time=time(NULL);  // Keep track critical section start time

		do{

			int id = mv->begin_trans();
			//printf("transaction created\n");
			int rand=getRand(g::m); // Gets a random no of iterations to be updated
			int local_value;
			float rand_time;

			for(int i=0;i<rand;i++)
			{
				int rand_index, rand_val;

				rand_index=getRand(g::m); 
				rand_val=getRand(g::const_val); 
				
				mv->read(id,rand_index,local_value); // It will read the value at the index rand_index in the local value
			//	printf("value read\n");
				
				auto g1 = std::chrono::high_resolution_clock::now();
				std::time_t now1 = std::chrono::system_clock::to_time_t(g1);
				std::strftime(buffer, 80, "%H:%M:%S", std::localtime(&now1));
				
				fprintf(g::log_file,"Thread id : %ld  Transaction :  %d reads from : %d a value of : %d at time:%s\n",pthread_self(),id,rand_index,local_value,buffer);
				local_value+=rand_val;

				// request to write the value into the transaction
				mv->write(id,rand_index,local_value);
			//	printf("value written\n");


				
				auto g2 = std::chrono::high_resolution_clock::now();
				std::time_t now2 = std::chrono::system_clock::to_time_t(g2);
				std::strftime(buffer, 80, "%H:%M:%S", std::localtime(&now2));
				
				fprintf(g::log_file,"Thread id : %ld  Transaction :  %d reads from : %d a value of : %d at time:%s\n",pthread_self(),id,rand_index,local_value,buffer);

				rand_time=getExpRand(g::lambda);
				usleep(rand_time*1000);

			}
			status = mv->trycommit(id);  // Try to commit the transaction
			//printf("Transaction commited\n");
			auto g3 = std::chrono::high_resolution_clock::now();
			std::time_t now3 = std::chrono::system_clock::to_time_t(g3);
			std::strftime(buffer, 80, "%H:%M:%S", std::localtime(&now3));
			fprintf(g::log_file,"Transaction :  %d try commits with result : %d at time:%s\n",id,status,buffer);
			if (status==-1)abort_count++;

		
				
		}while(status!=1);
		
	


		time_t end_time=time(NULL);

	g::total_delay[mTid]+=end_time-start_time;
	g::total_aborts[mTid]+=abort_count;
	}
}

int main()
{	//int K;
	//printf("Enter the value of k for k versions\n");
	//scanf("%d",&K);
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
	/*
    cout<<g::n <<endl;
    cout<<g::m <<endl;
    cout<<g::numTrans<<endl ;
    cout<<g::const_val<<endl;
    cout<<g::lambda<<endl ;
    */
	//printf("Input paramters read\n");

	// initializing the glabal vectord for storing the parameters

	for(int i=0;i<g::n;i++)
	{
		g::total_aborts.push_back(0);
		g::total_delay.push_back(0);
	}
	
	//printf("Global vectors initialized\n");
	

	// Starting the first scheduler
	std::thread tid[g::n];
	uint64_t res = 0;
	printf("----------Results for MVTO-------------\n");

	for(int i=0;i<g::n;i++){
		g::total_aborts.push_back(0);
		g::total_delay.push_back(0);
	}

	MVTO* mv =new MVTO(g::m);

	g::log_file=fopen("MVTO_log_file","w");
	fprintf(g::log_file,"----------Results for MVTO-------------\n");
	for(int i=0;i<g::n;i++){
		tid[i]=thread(udtMem,mv,i);
		}	
	for(int i=0;i<g::n;i++){
	    tid[i].join();
    }

    for(auto e: g::total_delay) res+=e;
     std::cout<<"Avg Delay:"<<((((double)res)/(g::n*g::numTrans))*1000)<<"(ms)"<<std::endl;
	 fprintf(g::log_file,"Avg Delay: %f\n",(((double)res)/(g::n*g::numTrans))) ;

     res = 0;

     for(auto e: g::total_aborts) res+=e;
    std::cout<<"Total aborts:"<<res<<std::endl;
	fprintf(g::log_file,"Total aborts: %ld\n",res) ;
     std::cout<<"Avg Aborts:"<<(((double)res)/(g::n*g::numTrans))<<std::endl;
	fprintf(g::log_file,"Avg aborts: %f",(((double)res)/(g::n*g::numTrans))) ;
	
	delete mv;

	fclose(g::log_file);
	return 0;

}