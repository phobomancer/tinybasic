#include<stdio.h>
#include<stdint.h>

#define programlen 40000
#define linelength 256


int KeywordCount = 11;
enum KeywordValues{PRINT = 1, IF, THEN, GOTO, INPUT, LET, GOSUB, RETURN, LIST, RUN, END};
char *Keywords[] = {"PRINT", "IF","THEN","GOTO","INPUT","LET","GOSUB","RETURN","LIST","RUN","END"};
int KeywordLengths[] = {5,2,4,4,5,3,5,6,4,3,3};

char programSpace[programlen];

int variables[26];

#define gosubStackLen 32
uint16_t gosubStack[gosubStackLen]; //32 seems like plently to me
int gosubStackIndex = -1; // will be incremented to point to return line on stack

uint16_t programStart = 0xffff;
uint16_t programStartLineNum = 0xffff;
uint16_t nextLine = 0xffff;
uint16_t currentLine = 0xffff;
int endProgramSpace = 0; //first firee byte in programSpace
int error = 0;   //the line number the error occured on or -1 if no current linenumber
int lineIndex;


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
// remove all whitespace and tokenize keywords (except in strings)
void TokenizeKeywords(char *string) {
	int inString=0;
	char newstr_index=0;
	for(int i=0;i<linelength && string[i]!=0;i++) {
		//check if a string is starting or ending, toggle the state
		// there should probably be away to escape a double quote here...
		if(string[i]=='"') {
			inString = !inString;
		}
		// don't mess with characters surrounded by double quotes, just copy
		// and move on
		if(inString) {
			string[newstr_index++] = string[i];
			continue; //loop is short circuited here everything below this can
			          //only be run if the we're not in a string
		}

		// test for all keywords starting at this location in the string
		// this is terribly brute force, but at least it's simple...i guesss
		for(int key = 0; key<KeywordCount ; key++){
			if(isKeyword(&string[i],key)){
				string[newstr_index++]=key+1; //key+1 is the keywords enum value
				i+=(KeywordLengths[key]); //minus one because i will still be incremented
				continue;
			}
		}
		// don't copy whitespace (including newlines
		if(string[i]!=' ' && string[i]!='\t' && string[i]!='\n'){
			string[newstr_index++] = string[i];
		}
	}
	if(inString) error=-1; // error must be nagative one because this function
	                       // runs  only only on program input not at run time

	string[newstr_index]=0; //terminate string
}

int variable(char *line) {
	if(line[lineIndex] >= 'A' && line[lineIndex] <= 'Z') {
		char var = line[lineIndex];
		lineIndex++;
		return variables[var - 'A'];
	} else {
		error = currentLine;
		return 1;
	}
}

int number(char *line) {
	int value=0;
	for(;line[lineIndex]>='0' && line[lineIndex]<='9';lineIndex++) {
		value*=10;
		value+=(line[lineIndex]-'0');
		if(line[lineIndex] != '0' && value == 0) {
			error = currentLine;
			return 0;
		}
	}
	return value;
}


int factor (char *line) {
	int value = 0;
	if(line[lineIndex]>='0' && line[lineIndex]<='9') {
		value=number(line);
		if(error) return 0;
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
		if(error) return 0;
	} else {
		error=currentLine;
	}
	return value;

}


int term (char *line) {
	int total = factor(line);
	int value;

	int operator;
	for(;line[lineIndex]=='*' || line[lineIndex]=='/';lineIndex++) {
		operator=line[lineIndex++];
		value=factor(line);
		lineIndex--;
		if(error) {
			return 0 ;
		}

		if(operator=='*'){
			total*=value;
		} else {
			total/=value;
		}
	}
	return total;
}


int expression(char *line) {
	int value = 0;
	int tmp;
	int currentOp = '+';
	if(line[lineIndex] == '-'){
		currentOp='-';
		lineIndex++;
	} else if (line[lineIndex] == '+' ) {
		lineIndex++;
	}
	for(;line[lineIndex] != 0; lineIndex++) {
		tmp = term(line);
		if(error) {
			return 0;
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
	return value;
}

int doPrint(char *line) {
	do {
		// this is mostly a check for characters separting the expression list
		// other than the print token or a comma
		if(line[lineIndex] != PRINT && line[lineIndex] != ',') {
			error=currentLine;
			return 0;
		}
		lineIndex++;
		if(line[lineIndex]=='"') {
			for(lineIndex++;line[lineIndex]!='"';lineIndex++) {
				if(line[lineIndex]==0) {
					error = currentLine;
					return -1;
				}
				putchar(line[lineIndex]);
			}
			lineIndex++; // point after 2nd double quote
		} else {
			int value = expression(line);
			if (error) {return 0;}
			printf("%d",value);
		}
	} while(line[lineIndex]==',');
	putchar('\n');
	return 0;
}

void doList() {
	for(uint16_t i=programStart; i != 0xffff; i = *(uint16_t *)(&programSpace[i+2])) {
		printf("%hu ",*(uint16_t*)&programSpace[i]);
		for(char *c = &programSpace[i+4]; *c != 0; c++) {
			if(*c <= KeywordCount) { // *c <= keywordcount must be a token
				printf("%s ",Keywords[(*c)-1]); // in that case detokenize
			} else {
				putchar(*c);
			}
		}
		putchar('\n');
	}
}

// this is kind of gross in that we start looking for the line
// at the top of the program every time even though most of the
// time the next line will be the next one in the list
// but GOTO, GOSUB, and RETURN make that not always the case
char *GetLine(uint16_t x) {
	for(uint16_t i=programStart; i != 0xffff; i = *(uint16_t *)(&programSpace[i+2])) {
		if(x == *(uint16_t*)&programSpace[i]) {
			return &programSpace[i];
		}
	}
	return NULL;
}

void doRun() {
	nextLine = *(uint16_t*)&programSpace[programStart];
	char *line;
	while(nextLine != 0xffff) {
		if(error) { return; }

		line = GetLine(nextLine);
		if(line == NULL) return;

		uint16_t nextlink = *(uint16_t*)&line[2];
		currentLine = *(uint16_t*)line; //set currentLine so we can report errors if any

		// if nextlink == 0xffff there is no next line in the program
		// so set next line to 0xffff to end the program (this could still be
		// modified by GOTO, GOSUB, or RETURN
		if(nextlink == 0xffff) {
			nextLine = 0xffff;
		} else {
			// if nextlink actually points somewhere use it to guess the next
			// line to be executed...again this might be modified but the
			// statements listed above
			nextLine = *(uint16_t*)&programSpace[nextlink];
		}
		lineIndex=0; // start processing this line at the begining
		doStatement(&line[4]); // &line[4] is the start of the text of the line
		                       // (after linenum and nextlink values)
		if(error) return;
	}

}

void doGoto(char *line) {
	int linenum = expression(line+1);
	if(error) return;
	// being a GOTO statement...set the next line to be executed
	nextLine =(uint16_t)linenum;
}

void doGosub(char *line) {
	gosubStackIndex++;
	if( gosubStackIndex >= gosubStackLen ){
		error = currentLine;
		return;
	}
	gosubStack[gosubStackIndex] = nextLine;
	int linenum = expression(line+1);
	nextLine =(uint16_t)linenum;
}

void doReturn() {
	if(gosubStackIndex < 0){
		error = currentLine;
		return;
	}
	nextLine = gosubStack[gosubStackIndex];
	gosubStackIndex--;
}

void doLet(char *line) {
	lineIndex++;
	if(line[lineIndex] < 'A' || line[lineIndex] > 'Z') {
		error = currentLine;
		return;
	}
	char var = line[lineIndex];
	lineIndex++;
	if(line[lineIndex] != '=') {
		error = currentLine;
		return;
	}
	lineIndex++;
	int value = expression(line);
	if(error) return;
	variables[var - 'A'] = value;
}


// treat =,<,> as flags, this function will be called twice and bitwise OR'd
// so at most 2 flags can be active at once
int getRelOp(char *line) {
	int op = 0;

	int partial=line[lineIndex];
	if( partial == '<') {
		op = 4;
	} else if ( partial ==  '>') {
		op = 2;
	} else if ( partial == '=') {
		op = 1;
	} else {
		error = currentLine;
	}
	return op;
}

void doIfThen(char *line) {
	lineIndex++;
	int value1, value2, op, tmp, istrue = 0;
	value1 = expression(line);
	if(error) return;

	op = getRelOp(line);
	if(error) return;

	lineIndex++;

	tmp = getRelOp(line); // test if there is a second character to relative operation
	if(tmp) {
		lineIndex++;
	} else {
		error = 0; // if no second character not actually an error
	}
	op|=tmp;
	value2 = expression(line);
	if(error) return;

	if(line[lineIndex] != THEN) {
		error = currentLine;
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

int doStatement(char *line) {
	switch(line[lineIndex]) {
		case 0:
			error = currentLine;
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
			nextLine = 0xffff;
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
int insertline(short linenumber, char *line) {
	uint16_t linestart = endProgramSpace;

	// write new line and the end of programSpace
	int isempty = (!*line);
	if(!isempty) {
		*(uint16_t *)(programSpace + endProgramSpace)=linenumber;
		*(uint16_t *)(programSpace + endProgramSpace + 2)=0xffff;
		for(uint16_t i = 0;;i++) {
			programSpace[endProgramSpace+4+i]=line[i];
			if(line[i] == 0){
				endProgramSpace+=i+5;
				break;
			}
		}
	}

	uint16_t currentLine, nextLine;
	uint16_t *currentNextLink, *nextNextLink;

	for(currentLine = 0, currentNextLink=&programStart;
	*currentNextLink != 0xffff;
	currentNextLink = nextNextLink, currentLine = nextLine) {
		// grab nextLine and nextNextLink to look ahead
		nextLine = *(uint16_t *) &programSpace[*currentNextLink];
		nextNextLink = (uint16_t *) &programSpace[*currentNextLink+2];
		if(linenumber <= nextLine ) break;
	}

	if(*currentNextLink == 0xffff) {  // append line
		*currentNextLink = linestart;
	} else if (linenumber < nextLine) { // insert line
		*(uint16_t *)(&programSpace[linestart+2])=*currentNextLink;
		*currentNextLink = linestart;
	} else if (linenumber == nextLine) { //replace line
		*(uint16_t *)(&programSpace[linestart+2])= *nextNextLink;
		*currentNextLink = linestart;
	}

}

int main(void) {
	char buffer[linelength];
	printf("WELCOME TO TINY BASIC IMPLEMENTED BY PHOBOMANCER\n(C)2025, ALL RIGHTS RESERVED\n");
	while( fgets(buffer, linelength, stdin) ) {
		int strpos = 0;
		error = 0;
		lineIndex = 0;
		currentLine = 0xffff;
		TokenizeKeywords(buffer);
		if( buffer[0] >= '0' && buffer[0]<='9'){
			int linenumber = number(buffer);
			insertline(linenumber,&buffer[lineIndex]);
		} else {
			int value = doStatement(buffer);
			if(error) {
				printf("ERROR ON LINE %d\n",error);
			}
			printf("\nOK\n");
		}
	}
	return 0;
}


