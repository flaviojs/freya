/* Bench code by nameless */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

/* ------------------------------- */
/* Function prototypes declaration */
/* ------------------------------- */
long calcFuncCall(void);
long calcAdd(void);
long calcSub(void);
long calcMul(void);
long calcDiv(void);
long calcShift(void);

long calcFloatAdd(void);
long calcFloatSub(void);
long calcFloatMul(void);
long calcFloatDiv(void);

long calcDoubleAdd(void);
long calcDoubleSub(void);
long calcDoubleMul(void);
long calcDoubleDiv(void);

int foo(void);

/* The function which does nothing */
int foo(void)
{
	return 1;
}
/* -------------------------------- */
/* Measure the call function        */
/* -------------------------------- */
long calcFuncCall(void)
{
	long stTim,endTim;
	int i,j;

	stTim=clock();

	for(i=0;i<10000;i++){
		for(j=0;j<10000;j++){
			foo();
		}
	}
	
	endTim=clock();
	endTim-=stTim;
	if(CLOCKS_PER_SEC>0){
		int time_dd=1000/CLOCKS_PER_SEC;
		if(time_dd>0) endTim*=time_dd;
	}

	return endTim;
}
/* ---------------------------------------- */
/* Measure the addition of an integer value */
/* ---------------------------------------- */
long calcAdd(void)
{
	long stTim,endTim;
	int i,j;
	int a;

	stTim=clock();

	a=0;
	for(i=0;i<10000;i++){
		for(j=0;j<10000;j++){
			a+=j;
		}
		a=0;
	}
	
	endTim=clock();
	endTim-=stTim;
	if(CLOCKS_PER_SEC>0){
		int time_dd=1000/CLOCKS_PER_SEC;
		if(time_dd>0) endTim*=time_dd;
	}

	return endTim;
}

/* ------------------------------------------- */
/* Measure the subtraction of an integer value */
/* ------------------------------------------- */
long calcSub(void)
{
	long stTim,endTim;
	int i,j;
	int a;

	stTim=clock();

	a=100000;
	for(i=0;i<10000;i++){
		for(j=0;j<10000;j++){
			a-=j;
		}
		a=0;
	}
	
	endTim=clock();
	endTim-=stTim;
	if(CLOCKS_PER_SEC>0){
		int time_dd=1000/CLOCKS_PER_SEC;
		if(time_dd>0) endTim*=time_dd;
	}

	return endTim;
}

/* ---------------------------------------------- */
/* Measure the multiplication of an integer value */
/* ---------------------------------------------- */
long calcMul(void)
{
	long stTim,endTim;
	int i,j;
	int a;

	stTim=clock();

	a=0;
	for(i=0;i<10000;i++){
		for(j=0;j<10000;j++){
			a*=j;
		}
	}
	
	endTim=clock();
	endTim-=stTim;
	if(CLOCKS_PER_SEC>0){
		int time_dd=1000/CLOCKS_PER_SEC;
		if(time_dd>0) endTim*=time_dd;
	}

	return endTim;
}

/* ---------------------------------------- */
/* Measure the division of an integer value */
/* ---------------------------------------- */
long calcDiv(void)
{
	long stTim,endTim;
	int i,j;
	int a;

	stTim=clock();

	a=0;
	for(i=0;i<10000;i++){
		for(j=1;j<10001;j++){	/* To not generate a division by 0, j starts at 1 */
			a/=j;
		}
	}
	
	endTim=clock();
	endTim-=stTim;
	if(CLOCKS_PER_SEC>0){
		int time_dd=1000/CLOCKS_PER_SEC;
		if(time_dd>0) endTim*=time_dd;
	}

	return endTim;
}

/* ------------------------------------- */
/* Measure the shift of an integer value */
/* ------------------------------------- */
long calcShift(void)
{
	long stTim,endTim;
	int i,j;
	int a,b;

	stTim=clock();

	a=0;
	b=1;
	for(i=0;i<10000;i++){
		for(j=0;j<10000;j++){
			a<<=b;
		}
		a=0;
	}
	
	endTim=clock();
	endTim-=stTim;
	if(CLOCKS_PER_SEC>0){
		int time_dd=1000/CLOCKS_PER_SEC;
		if(time_dd>0) endTim*=time_dd;
	}

	return endTim;
}

/* ------------------------------------- */
/* Measure the addition of a float value */
/* ------------------------------------- */
long calcFloatAdd(void)
{
	long stTim,endTim;
	int i,j;
	float a;

	stTim=clock();

	a=0.0f;
	for(i=0;i<10000;i++){
		for(j=0;j<10000;j++){
			a+=j;
		}
		a=0.0f;
	}
	
	endTim=clock();
	endTim-=stTim;
	if(CLOCKS_PER_SEC>0){
		int time_dd=1000/CLOCKS_PER_SEC;
		if(time_dd>0) endTim*=time_dd;
	}

	return endTim;
}

/* ---------------------------------------- */
/* Measure the subtraction of a float value */
/* ---------------------------------------- */
long calcFloatSub(void)
{
	long stTim,endTim;
	int i,j;
	float a;

	stTim=clock();

	a=100000.0f;
	for(i=0;i<10000;i++){
		for(j=0;j<10000;j++){
			a-=(float)j;
		}
		a=0.0f;
	}
	
	endTim=clock();
	endTim-=stTim;
	if(CLOCKS_PER_SEC>0){
		int time_dd=1000/CLOCKS_PER_SEC;
		if(time_dd>0) endTim*=time_dd;
	}

	return endTim;
}


/* ------------------------------------------- */
/* Measure the multiplication of a float value */
/* ------------------------------------------- */
long calcFloatMul(void)
{
	long stTim,endTim;
	int i,j;
	float a;

	stTim=clock();

	a=0.0f;
	for(i=0;i<10000;i++){
		for(j=0;j<10000;j++){
			a*=(float)j;
		}
	}
	
	endTim=clock();
	endTim-=stTim;
	if(CLOCKS_PER_SEC>0){
		int time_dd=1000/CLOCKS_PER_SEC;
		if(time_dd>0) endTim*=time_dd;
	}

	return endTim;
}

/* ------------------------------------- */
/* Measure the division of a float value */
/* ------------------------------------- */
long calcFloatDiv(void)
{
	long stTim,endTim;
	int i,j;
	float a;

	stTim=clock();

	a=0.0f;
	for(i=0;i<10000;i++){
		for(j=1;j<10001;j++){	/* To not generate a division by 0, j starts at 1 */
			a/=(float)j;
		}
	}
	
	endTim=clock();
	endTim-=stTim;
	if(CLOCKS_PER_SEC>0){
		int time_dd=1000/CLOCKS_PER_SEC;
		if(time_dd>0) endTim*=time_dd;
	}

	return endTim;
}

/* ------------------------------------------------------ */
/* Measure the addition of a double precision float value */
/* ------------------------------------------------------ */
long calcDoubleAdd(void)
{
	long stTim,endTim;
	int i,j;
	double a;

	stTim=clock();

	a=0.0;
	for(i=0;i<10000;i++){
		for(j=0;j<10000;j++){
			a+=j;
		}
		a=0.0;
	}
	
	endTim=clock();
	endTim-=stTim;
	if(CLOCKS_PER_SEC>0){
		int time_dd=1000/CLOCKS_PER_SEC;
		if(time_dd>0) endTim*=time_dd;
	}

	return endTim;
}

/* --------------------------------------------------------- */
/* Measure the subtraction of a double precision float value */
/* --------------------------------------------------------- */
long calcDoubleSub(void)
{
	long stTim,endTim;
	int i,j;
	double a;

	stTim=clock();

	a=100000.0f;
	for(i=0;i<10000;i++){
		a=10000.0;
		for(j=0;j<10000;j++){
			a-=(double)j;
		}
		a=0.0;
	}
	
	endTim=clock();
	endTim-=stTim;
	if(CLOCKS_PER_SEC>0){
		int time_dd=1000/CLOCKS_PER_SEC;
		if(time_dd>0) endTim*=time_dd;
	}

	return endTim;
}


/* ------------------------------------------------------------ */
/* Measure the multiplication of a double precision float value */
/* ------------------------------------------------------------ */
long calcDoubleMul(void)
{
	long stTim,endTim;
	int i,j;
	double a;

	stTim=clock();

	a=0.0;
	for(i=0;i<10000;i++){
		for(j=0;j<10000;j++){
			a*=(double)j;
		}
	}
	
	endTim=clock();
	endTim-=stTim;
	if(CLOCKS_PER_SEC>0){
		int time_dd=1000/CLOCKS_PER_SEC;
		if(time_dd>0) endTim*=time_dd;
	}

	return endTim;
}

/* ------------------------------------------------------ */
/* Measure the division of a double precision float value */
/* ------------------------------------------------------ */
long calcDoubleDiv(void)
{
	long stTim,endTim;
	int i,j;
	double a;

	stTim=clock();

	a=0.0;
	for(i=0;i<10000;i++){
		for(j=1;j<10001;j++){
			a/=(double)j;		/* To not generate a division by 0, j starts at 1 */
		}
	}
	
	endTim=clock();
	endTim-=stTim;
	if(CLOCKS_PER_SEC>0){
		int time_dd=1000/CLOCKS_PER_SEC;
		if(time_dd>0) endTim*=time_dd;
	}

	return endTim;
}

int main()
{
	char szStr[256];
	int ctime;
	ctime=0;

	ctime=ctime+calcFuncCall();
	ctime=ctime+calcAdd();
	ctime=ctime+calcSub();
	ctime=ctime+calcMul();
	ctime=ctime+calcDiv();
	ctime=ctime+calcShift();
	ctime=ctime+calcFloatAdd();
	ctime=ctime+calcFloatSub();
	ctime=ctime+calcFloatMul();
	ctime=ctime+calcFloatDiv();
	ctime=ctime+calcDoubleAdd();
	ctime=ctime+calcDoubleSub();
	ctime=ctime+calcDoubleMul();
	ctime=ctime+calcDoubleDiv();


	printf("Execution Time=,%d",ctime);

	return 0;
}
