#include<stdio.h>
#define programlen 40000
#define linelength 256
int KeywordCount = 11;
enum KeywordValues{PRINT = '?', IF='!', THEN='=' ,GOTO='%', INPUT='&', LET='@', GOSUB='\\', RETURN='~', LIST='`', RUN='_', END='#'};
int KeywordLookup[] = {PRINT, IF,THEN,GOTO,INPUT,LET,GOSUB,RETURN,LIST,RUN,END};
char *Keywords[] = {"PRINT", "IF","THEN","GOTO","INPUT","LET","GOSUB","RETURN","LIST","RUN","END"};
int KeywordLengths[] = {5,2,4,4,5,3,5,6,4,3,3};
char programspace[programlen];
int variables[26];

#define gosubStackLen 32
unsigned short gosubStack[gosubStackLen]; //32 seems like plently to me

int gosubStackIndex = -1;
unsigned short programstart = 0xffff;
unsigned short nextline= 0xffff;
unsigned short programstartlinenum = 0xffff;
int endprogramspace = 0; //first firee byte in programspace
int error = 0;
int lineIndex;
unsigned short link;

int doStatement(char *line); 
int expression(char *line); 

int isKeyword(const char *a, int key) {
//	printf("testing %s\n",Keywords[key]);
	for(int i = 0; i < KeywordLengths[key]; i++){
		if(a[i]!=Keywords[key][i]) {
			return 0;
		}
	}
	return 1;
}

// the resulting string can only be shorter or equal to 
// the original string so we can do this inplace
void TokenizeKeywords(char *string) {
	int inString=0;
	char newstr_index=0;
	for(int i=0;i<linelength && string[i]!=0;i++) {
		if(string[i]=='"') {
			inString = !inString;
		}
		if(inString) {
			string[newstr_index++] = string[i];
			continue;
		}
		for(int key = 0; key<KeywordCount ; key++){
			if(isKeyword(&string[i],key)){
				string[newstr_index++]=KeywordLookup[key];
				i+=(KeywordLengths[key]); //minus one because i will still be incremented
				continue;
			}
		}
		if(string[i]!=' ' && string[i]!='\t' && string[i]!='\n'){
			string[newstr_index++] = string[i];
		}
	}
	string[newstr_index]=0;
}
int variable(char *line) {
	if(line[lineIndex] >= 'A' && line[lineIndex] <= 'Z') {
		char var = line[lineIndex];
		lineIndex++;
		return variables[var - 'A'];
	} else {
		error = 1;
		return 1;
	}
}

int number(char *line) {
	//printf("in number\n");
	int value=0;
	for(;line[lineIndex]>='0' && line[lineIndex]<='9';lineIndex++) {
		value*=10;
		value+=(line[lineIndex]-'0');
	}
	//printf("number %d\n",value);
	return value;
}


int factor (char *line) {
	//printf("in factor\n");
	int value = 0;
	if(line[lineIndex]>='0' && line[lineIndex]<='9') {
		value=number(line);
	} else if (line[lineIndex] == '(') {
		lineIndex++;
		value=expression(line);
		if (line[lineIndex] == ')') {
			lineIndex++;
		} else {
			error=-1;
			return error;
		}
	} else if(line[lineIndex] >= 'A' && line[lineIndex] <= 'Z') {
		value = variable(line);
	} else {
		//printf("factor error\n");
		error=-1;
	}
	//printf("factor char %c\n",line[lineIndex]);
	//printf("factor %d\n",value);
	return value;
	
}


int term (char *line) {
	//printf("in term\n");
	int total = factor(line);
	int value;
	
	int operator;
	for(;line[lineIndex]=='*' || line[lineIndex]=='/';lineIndex++) {
		operator=line[lineIndex++];
		value=factor(line);
		lineIndex--;
		if(error) { 
			return error ;
		}
		
		if(operator=='*'){
			total*=value;
		} else {
			total/=value;
		}
	}
	//printf("term char %c\n",line[lineIndex]);
	//printf("term %d\n",total);
	return total;
}


int expression(char *line) {
	int value = 0;
	int tmp;
	int currentOp = '+';
	//printf("in expression\n");
	if(line[lineIndex] == '-'){
		currentOp='-';
		lineIndex++;
	} else if (line[lineIndex] == '+' ) {
		lineIndex++;
	}
	for(;line[lineIndex] != 0; lineIndex++) {
		tmp = term(line);
		//printf("expression char: %c\n",line[lineIndex]);
		if(error) {
			return error;
		}
		
		if(currentOp == '-') {
			tmp = 0-tmp;
		}
		value += tmp;
		if(line[lineIndex] == '-') {
			currentOp = '-';
		} else if(line[lineIndex] == '+') {
			currentOp = '+';
		} else {
			break;
		}
	}
	//printf("expression %d\n",value);
	return value;
}

int doPrint(char *line) {
	do {
		lineIndex++;
		if(line[lineIndex]=='"') {
			for(lineIndex++;line[lineIndex]!='"';lineIndex++) {
				if(line[lineIndex]==0) {
					error = -1;
					return -1;
				}
				putchar(line[lineIndex]);
			}
			lineIndex++; // point after 2nd double quote
		} else {
			int value = expression(line);
			if (error) {return -1;}
			printf("%d",value);
		}
	} while(line[lineIndex]==',');
	putchar('\n');
	return 0;
}
		
void doList() {
	//printf("in list\n");
	for(int i = 0; i < 30; i++) {
		//printf("%2.2hhx ", programspace[i]);
	}

	for(unsigned short i=programstart; i != 0xffff; i = *(unsigned short *)(&programspace[i+2])) {
		printf("%hu %s\n",*(unsigned short*)&programspace[i], &programspace[i+4]);
	}
}

char *GetLine(unsigned short x) {
	for(unsigned short i=programstart; i != 0xffff; i = *(unsigned short *)(&programspace[i+2])) {
		if(x == *(unsigned short*)&programspace[i]) {
			return &programspace[i];
		}
	}
	return NULL;
}

void doRun() {
	nextline = *(unsigned short*)&programspace[programstart];
	//printf("nextline %hu\n",nextline);
	char *line;
	while(nextline != 0xffff) {
		if(error) { return; }
		line = GetLine(nextline);
		if(line == NULL) return;
		//printf("getting nextlink\n");
		unsigned short nextlink = *(unsigned short*)&line[2];
		//printf("next link %hu\n",nextlink);
		if(nextlink == 0xffff) {
			nextline = 0xffff;
		} else {
			nextline = *(unsigned short*)&programspace[nextlink];
		}
		//printf("next line %hu\n",nextline);
		lineIndex=0;
		//printf("running doStatement\n");
		doStatement(&line[4]);
	}
		
}

void doGoto(char *line) {
	int linenum = expression(line+1);
	nextline =(unsigned short)linenum;
}

void doGosub(char *line) {
	gosubStackIndex++;
	if( gosubStackIndex >= gosubStackLen ){
		error = 1;
		return;
	}
	gosubStack[gosubStackIndex] = nextline;
	int linenum = expression(line+1);
	nextline =(unsigned short)linenum;
}

void doReturn() {
	if(gosubStackIndex < 0){
		error = 1;
		return;
	}
	nextline = gosubStack[gosubStackIndex];
	gosubStackIndex--;
}

void doLet(char *line) {
	lineIndex++;
	if(line[lineIndex] < 'A' || line[lineIndex] > 'Z') {
		error = 1;
		return;
	}
	char var = line[lineIndex];
	lineIndex++;
	if(line[lineIndex] != '=') {
		error = 1;
		return;
	}
	lineIndex++;
	int value = expression(line);
	variables[var - 'A'] = value;
}


int getRelOp(char *line) {
	int op = 0;

	int partial=line[lineIndex];
	if( partial == '<') {
		op = 4;
	} else if ( partial ==  '>') {
		op = 2;
	} else if ( partial == '=') {
		op = 1;
	}
	return op;
}
void doIfThen(char *line) {
	lineIndex++;
	int value1, value2, op, tmp, istrue = 0;
	value1 = expression(line);
	op = getRelOp(line);
	if(op == 0) {
		error = 1;
		return;
	}
	lineIndex++;
	tmp = getRelOp(line);
	if(tmp) {
		lineIndex++;
	}
	op|=tmp;
	value2 = expression(line);

	if(line[lineIndex] != THEN) {
		error = 1;
		return;
	} else {
		lineIndex++;
	}

	if(value1 == value2 && (op&1)){
		istrue=1;
	} else if (value1 > value2 && (op&2)){
		istrue=1;
	} else if (value1 < value2 && (op&4)){
		istrue=1;
	}
	
	if(istrue) {
		doStatement(line);
	}
}
	
	
//int KeywordLookup[] = {PRINT, IF,THEN,GOTO,INPUT,LET,GOSUB,RETURN,LIST,RUN,END};
int doStatement(char *line) {
	switch(line[lineIndex]) {
		case 0:
			error = -1;
			return -1;
		case PRINT:
			doPrint(line);
			break;
		case LIST:
			doList();
			break;
		case RUN:
			doRun();
			break;
		case GOTO:
			doGoto(line);
			break;
		case LET:
			doLet(line);
			break;
		case IF:
			doIfThen(line);
			break;
		case END:
			nextline = 0xffff;
			break;
		case GOSUB:
			doGosub(line);
			break;
		case RETURN:
			doReturn();
			break;
	}
	return 0;
}
//in program space the contents will be
// linenumber linknumber line
// i'm not proud of the code here it feels clunky
int insertline(short linenumber, char *line) {
	unsigned short currentline = 0xffff;
	unsigned short currentlink = 0xffff;
	unsigned short linestart = endprogramspace;
	
	*(unsigned short *)(programspace + endprogramspace)=linenumber;
	*(unsigned short *)(programspace + endprogramspace + 2)=0xffff;
	for(unsigned short i = 0;;i++) {
		//printf("es: %hu i: %hu",endprogramspace, i);
		programspace[endprogramspace+4+i]=line[i];
		if(line[i] == 0){
			endprogramspace+=i+5;
			break;
		}
	}
	//printf("linenumber %hu\n", linenumber);
	if(linenumber <= programstartlinenum){
			//printf("first line\n");
		if(linenumber == programstartlinenum) {	
			*(unsigned short *)(&programspace[linestart+2]) = *(unsigned short*)&programspace[programstart+2];
		} else {
			*(unsigned short *)(&programspace[linestart+2]) = programstart;
		}
		programstart=linestart;
		programstartlinenum = linenumber;	
	} else {
		for(unsigned short i=programstart; i != 0xffff; i = *(unsigned short *)(&programspace[i+2])) {
			//printf("iteration\n");
			currentline = *(unsigned short *)(&programspace[i]);
			currentlink = *(unsigned short *)(&programspace[i+2]);	
			//printf("current done %hu \n", currentlink);
			//printf("next done\n");
			if(currentlink!=0xffff && linenumber == *(unsigned short *)&programspace[currentlink]) {
				//printf("same line num\n");
				unsigned short nextline = *(unsigned short *)(&programspace[currentlink]);
				unsigned short nextlink = *(unsigned short *)(&programspace[currentlink+2]);
				*(unsigned short *)(&programspace[i+2])=linestart;
				*(unsigned short *)(&programspace[linestart+2])=nextlink;
				//printf("start %hu, link %hu\n",linestart,nextlink);
				break;
			}
			if(currentline < linenumber && (currentlink==0xffff || linenumber < *(unsigned short *)&programspace[currentlink])) {
				//printf("inserting\n");
				*(unsigned short *)(&programspace[i+2])=linestart;
				*(unsigned short *)(&programspace[linestart+2])=currentlink;
				
				//printf("start %hu, link %hu\n",linestart,nextlink);
				break;
			}
			//printf("end of loop\n");
		}
	}
}

int main(void) {
	char buffer[linelength];
	printf("WELCOME TO TINY BASIC IMPLEMENTED BY PHOBOMANCER\n(C)2025, ALL RIGHTS RESERVED\n");
	while( fgets(buffer, linelength, stdin) ) {
		int strpos = 0;
		error = 0;
		lineIndex=0;
		TokenizeKeywords(buffer);
		if( buffer[0] >= '0' && buffer[0]<='9'){
			//printf("inserting line\n");
			int linenumber = number(buffer);
			insertline(linenumber,&buffer[lineIndex]);
		} else {
			//printf("%s\n",buffer);
			//printf("%s\n",buffer);
			int value = doStatement(buffer);
			if(error) {printf("error\n");}
			printf("\nOK\n");
		}		
	}
	return 0;
}


