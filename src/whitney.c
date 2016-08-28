#include <stdio.h>
#include <string.h>
#include <stdlib.h>


/* 
   character type - used to process user input 
*/
typedef char C;


/* 
   I represents our numbers 
*/
typedef long I;


/* 
A: Our generic array container

From An APL Emulator:
    t  : type (0 is standard array, 1 is boxed array)
    r  : rank (0-3)
    d  : dimensions
    p  : data storage in two flavors, following An APL Emulator design. 

For explanation of why p has two elements, see commentary on "from" operator and ga().
*/
typedef struct a {I t, r, d[3], p[2];} *A;


#define P printf
#define R return


/* 
   Unary and binary function application 
*/
#define V1(f) A f(A w)
#define V2(f) A f(A a, A w)


/* 
   Perform x n times - x provides an environment where i refers to current iteration 
*/
#define DO(n,x) {I i=0,_n=(n);for(;i<_n;++i){x;}}


/* 
   ma: produce a new location 
*/
I *ma(n)
{
	R (I*)malloc(n*sizeof(I));
}


/* 
   mv: Copy n elements in s to d.  
*/
mv(I *d,I *s, int n) 
{
	DO(n,d[i]=s[i]);
}


/* 
   tr: compute the raveled dimension (trace) of A 
*/
tr(int r,I *d)
{
	I z = 1;
	DO(r,z=z*d[i]); 

	R z;
}


/* 
   ga: generate array 
*/
A ga(int t,int r,I *d)
{
    
        /* 
	   From An APL Emulator:
	   
	   [ S P | D |    V ]   or   [ S P |     A    ]
	   
	   "P specifies whether variable has value or not. If variable has value it may
	   be specified by D or V (scalar characters or shorts specified this way) or the
	   value may be specified by the block of memory beginning at A-4"
	   
	   In the malloc context below, we allocate 5 cells for the type, rank, and dimension 
	   list of A, leaving the rest to be populated by the result of the trace call. In the 
	   case of rank 0 noun, p[0] contains the digit. For all other nouns, the cells counted 
	   by tr provide room for the full, unraveled matrix. In other words, the degenerate 
	   case of a rank 0 matrix represents [ SP | D V ]. For other matrices, p will just 
	   refer to the first two elements." 
        */
    
        /* Because tr is always at least 1, each A has at least 3 p cells. */
        A z=(A)ma(5+tr(r,d));
	
	z->t=t;
	z->r=r;
	mv(z->d,d,r);
	R z;
}

V1(iota)
{
	I n=*w->p;      /* w is rank 0. get its content in the first element of p */
	A z=ga(0,1,&n); /* allocate a 1D matrix of length n */
	DO(n,z->p[i]=i);
	R z;
}


V2(plus)
{
        /* e.g. a=~4; 3+a; a+3 */
        /* don't try this with boxed arrays */

	I r=w->r;      
	I *d=w->d;
	
	I n=tr(r,d); 
	A z=ga(0,r,d);

	/* Out of range entries default to 0 */
	DO(n,z->p[i]=a->p[i]+w->p[i]); 
	R z;
}


/* from: a form of APL's take 
e.g 
 * rank 1 case:
    a=1,2,3
    0{a => 1
    1{a => 2

 * rank>1 case:
    a=2,2 
    b=a#~4
    1{b => 2 3 
*/ 
V2(from)
{
	I r=w->r-1;         /* reduce rank by 1 because we're slicing  */
	I *d=w->d+1;        /* advance beyond first dimension of w (second array) */
	I n=tr(r,d);        /* use these remaining dimensions to compute size of array.
			       For rank 0 nouns, r=-1 */
	A z=ga(w->t,r,d);   /* maintin w's type */
    
    /* 
     w->p+(n * *a->p) breaks down as: "In e{f, *a->p equals the value of e 
     (single number - 2,3{~6 doesn't work as expected). Then n**a->p computes 
     the necessary offset in w->p. Take these examples:
     
     > a=~3
     > 1{a => 1
     
     w->p points to start of {0,1,2}. w->d=3, so if w->d pointed to 3 in {3,0,0}, 
     then d=w->d+1=0. w->r=1, so r=0. Then n=tr(0,0)=1. For an unboxed matrix, 
     this makes z=ga(0,0,0). Because *a->p=1 (we can only take from a single value, 
     unlike APL), n**a->p=1, which returns the second element in w->p. This explains 
     the need for a default second element in p; it maintains consistency for 
     rank-reduction operations.
     
     Notice too that from returns 0 for rank 0 matrices if a>0. e.g.
     0{2 => 2
     1{2 => 0
     9{2 => 0
     
     Why? For rank 0 matrices, r=-1. This makes the call to to ga equivalent to ga(0,0,0). 
     When a->p>0, z->p will be populated with all 0s. So, calls to pr will show z->p=0. 
     Furthermore, because from always returns a rank 0 matrix when w is rank 0, this process 
     can recur. You can call from as much as you like with the same result, e.g., 1{9{4{5{3{6 => 0 
    */
	mv(z->p,w->p+(n**a->p),n); 
	R z;
}


/* boxed array: pointer to A */
/* you can demonstrate this using plus, e.g.: e=<~3; e+1 => returns address of e plus one */
V1(box)
{
	A z=ga(1,0,0);
	*z->p=(I)w;
	R z;
}


/* cat(A a, A w): catenate w onto a a rank-1 noun A,B. */
V2(cat)
{
	I an=tr(a->r,a->d);
	I wn=tr(w->r,w->d);
	I n=an+wn; 
	A z=ga(w->t,1,&n);
	mv(z->p,a->p,an);
	mv(z->p+an,w->p,wn);
	R z;
}


V2(find){}


/* rsh: reshape dimension of w according to elements of a (rank 1) */
/* TODO: check with APL's behavior to see if it's consistent about using rank 1 matrices */
V2(rsh)
{
        /* 
	   If not rank 0, the rank of the result will be the same as the element count 
	   of a. Otherwise, we'll produce a rank 1 matrix 
	*/
	I r=a->r?*a->d:1; 
	
	/* 
	   Total element count in matrix with dimensions a is tr(r,a->p). For example, 
	   if a=2,2, total element count is 4 (rank 1 * entries (2*2)) 
	*/
	I n=tr(r,a->p);

	/* 
	   Total element count in w. The asymmetry between this and previous line 
	   depends on a being rank 0 or 1. 
	*/
	I wn=tr(w->r,w->d); 

	/* 
	   Similar to the calculation of n, r, a->p gives the total element count 
	   of an a-sized matrix. This allocates cells in a new matrix appropriately 
	   sized for this new shape */
	A z=ga(w->t,r,a->p); 

	/* 
	   First move the elements of w into z. Do a check of dimension mismatch 
	   between a and w for the next step (pick smaller of wn, n) 
	*/
	mv(z->p,w->p,wn=n>wn?wn:n); 


	/* 
	   Conditional branch, pulling duplicated contents of already-populated z->p. Cute.
	*/
	if(n-=wn) 
		mv(z->p+wn,z->p,n); 
	R z;
}

/* sha: shape */
V1(sha)
{
	A z=ga(0,1,&w->r);
	mv(z->p,w->d,w->r); 
	R z;
}

/* identity */
V1(id)
{
	R w;
}

/* 
   { - size of the array - 1 for rank==0, first dimension "0{#x" for rank>0 
*/
V1(size)
{
	A z=ga(0,0,0);
	*z->p=w->r?*w->d:1;
	R z;
}

/* pr: print integer i */
pi(i)
{
	P("%d ",i);
}

/* nl: print newline  */
nl()
{
	P("\n");
}

pr(A w)
{
	I r=w->r;
	I *d=w->d;
	I n=tr(r,d);

	/* 
	   Print nouns dimensions - rank 0 nouns do not print dimensions because of 
	   DOs terimination conditions.
	*/
	DO(r,pi(d[i])); nl(); 

	/* 
	   Special case of a boxed array. Note recursive call to pr.
	*/
	if(w->t) {
		DO(n,P("< "); pr(w->p[i])); } 
	else 
		DO(n,pi(w->p[i]));nl();
}

/* verb table */
C vt[]="+{~<#,";

/* vd : dyadic verb lookup table */
A(*vd[])()={0,plus,from,find,0,rsh,cat};

/* vm: monadic verb lookup table */
A (*vm[])()={0,id,size,iota,box,sha,0};

/* st: single character noun storage state table */
I st[26]; 

/* qp: quoted printable/pronoun? */
qp(int a)
{
	R  a>='a'&&a<='z';
}

/* qv: quotable value */
qv(int a)
{
	R a<'a';
}

/* ex: evaluator */
A ex(I *e)
{
	I a = *e;
	if(qp(a))
	{
		if(e[1] == '=')
			R st[a-'a'] = ex(e+2);
		a = st[ a-'a'];
	}
	
	R qv(a)?(*vm[a])(ex(e+1)):e[1]?(*vd[e[1]])(a,ex(e+2)):(A)a;
}

/* noun: produce a scalar (array of rank 0) between 0-9 */
noun(c)
{
	A z;
	if(c<'0'||c>'9')
		R 0;
	z=ga(0,0,0);
	*z->p=c-'0';
	R z;
}

/* verb: if operation in symbol table vt, return its index; return 0 otherwise */
verb(c)
{
	I i=0;
	for(;vt[i];)
		if(vt[i++]==c)
			R i;
	R 0;
}

/* wd: basic reader */
I *wd(C *s)
{
	I a;
	I n=strlen(s);
	I *e=ma(n+1);
	 C c;

	DO(n,e[i]=(a=noun(c=s[i]))?a:(a=verb(c))?a:c);
	e[n]=0;
	R e; 
}

main()
{
	C s[99];
	while(gets(s)) {
        pr(ex(wd(s)));
    }

}
