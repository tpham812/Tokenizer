#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Tokenizer structure */
struct TokenizerT_ {

	char *delims; 		/* Hold a copy of delimiter inputted from the shell */
	char *tokenStream;	/* Hold a copy of the token stream inputted from the shell */
	char *ppdelims; 	/* Holds the delimiter after pre-parsing */
	char *pptokenStream; 	/* Holds the token stream after pre-parsing */
	int position; 		/* Position of the start of a new token */
};

typedef struct TokenizerT_ TokenizerT;

/* Create tokenizer. Returns NULL if allocation of structure and its members fails. Otherwise return the pointer to tokenizer */
TokenizerT *TKCreate(char *separators, char *ts) {
	
	int stringLength;

	TokenizerT *tokenizer = (TokenizerT*)malloc(sizeof(TokenizerT));
	if(tokenizer == NULL)
		return NULL; /* Returns NULL if allocation fails */

	/* Allocate memory for delimis member of tokenizer to store copy of delimiters that was inputted */
	stringLength = strlen(separators) + 1;
	tokenizer -> delims = (char*)malloc(sizeof(char) * stringLength);
	if(tokenizer -> delims == NULL)
		return NULL; /* Returns NULL if allocation fails */
	memcpy(tokenizer -> delims, separators, stringLength - 1);
	tokenizer -> delims[stringLength-1] = '\0';
	
	/* Allocate memory for tokenStream member of tokenizer to store copy of token stream that was inputted */
	stringLength = strlen(ts) + 1;
	tokenizer -> tokenStream = (char*)malloc(sizeof(char) * stringLength);
	if(tokenizer -> tokenStream == NULL)
		return NULL; /* Returns NULL if allocation fails */
	memcpy(tokenizer -> tokenStream, ts, stringLength - 1);
	tokenizer -> tokenStream[stringLength-1] = '\0';

	tokenizer -> position = 0; /* Set position of traversal to beginning character of token stream */
	tokenizer -> pptokenStream = NULL; /* Set tokenStream to NULL */

	return tokenizer;
}

/* Free tokenizer and the members that were allocated */
void TKDestroy(TokenizerT *tk) {

	free(tk -> delims);	
	free(tk -> tokenStream);
	free(tk -> ppdelims);
	free(tk -> pptokenStream);
	tk -> delims = NULL;
	tk -> tokenStream = NULL;
	tk -> ppdelims = NULL;
	tk -> pptokenStream = NULL;
	free(tk);
}

/* Function to check if character in token passed is a delimiter. Return 1 if the character is a delimiter, 0 if not. */
int isADelimiter(char character, char *delims) {

	int i = 0, found = 0;

	while(delims[i] != '\0') {
		if(delims[i] == character) {
			found = 1;
			return found;
		}
		i++;
	}
	return found;
}

/* Skip position in token stream if it is one of a token characters. (Find where the token ends in the stream) */
int skipTokenPos(char *tokenStream, char *delims, int position) {

	/* Checks to see if it is a delimiter or token character, if it is not delimiter, increment position. */
	while(isADelimiter(tokenStream[position], delims) == 0 && tokenStream[position] != '\0')
		position++;

	return position;

}
/* Skip position in token stream if it is delimiter (Find where the token begins in the stream) */ 
int skipDelimiterPos(char *tokenStream, char *delims, int position) {

	/* Checks to see if it is a delimiter or token character, if it is delimiter, increment position. */
	while(isADelimiter(tokenStream[position], delims) == 1 && tokenStream[position] != '\0')
		position++;

	return position;
}

/* Function to extract token and store it in a new string and return it */
char *createToken(char *tokenStream, int position, int nextPosition) {
	
	char *newToken;
	newToken = (char*)malloc(sizeof(char) * (nextPosition - position + 1));
	memcpy(newToken, tokenStream + position, nextPosition - position);
	newToken[nextPosition - position] = '\0';
	return newToken;
}

/* Function to count the number of escape characters in the token stream */
int findNumOfESC(char *string) {

	int i, foundBS, numOfEscChar;
	i = 0, foundBS = 0, numOfEscChar = 0;
	while(string[i] != '\0') {
		if(string[i] == '\\') {
			foundBS = 1;
			i++;
			continue;
		}
		if(foundBS == 1) { 
			if(string[i] == 'n' || string[i] == 't' || string[i] == 'v' || string[i] == 'b' || 				   				string[i] == 'r' || string[i] == 'f' || string[i] == 'a' || string[i] == '\\' || string[i] == '"') {
				foundBS = 0; 	/* Reset foundBS if escape character is found */
				numOfEscChar++;	/* Increment the counter if escape character exists */
			}
			else
				foundBS = 0;
		}
		i++;
	}
	return numOfEscChar;
}

void replaceEscChar(char *string, char *preparseString) {

	int i, j, foundBS;
	i = 0; j = 0; foundBS = 0;
	/* Traverse token to find the 2 byte escape characters representation and store 1 byte 
	representation into new string if exists */
	while (string[i] != '\0') {
		if(string[i] == '\\' && foundBS == 0) 
			{ foundBS = 1; i++; continue; }

		if(string[i] == 'n' && foundBS == 1) 
			{ preparseString[j] = '\n'; foundBS = 0; j++; }

		else if(string[i] == 't' && foundBS == 1) 
			{ preparseString[j] = '\t'; foundBS = 0; j++; }

		else if(string[i] == 'v' && foundBS == 1) 
			{ preparseString[j] = '\v'; foundBS = 0; j++; }

		else if(string[i] == 'b' && foundBS == 1) 
			{ preparseString[j] = '\b'; foundBS = 0; j++; }

		else if(string[i] == 'r' && foundBS == 1) 
			{ preparseString[j] = '\r'; foundBS = 0; j++; }

		else if(string[i] == 'f' && foundBS == 1) 
			{ preparseString[j] = '\f'; foundBS = 0; j++; }

		else if(string[i] == 'a' && foundBS == 1) 
			{ preparseString[j] = '\a'; foundBS = 0; j++; }

		else if(string[i] == '\\' && foundBS == 1)
			{ preparseString[j] = '\\'; foundBS = 0; j++; }

		else if(string[i] == '"' && foundBS == 1) 
			{ preparseString[j] = '\"'; foundBS = 0; j++; }

		/* Store everything else which does not have a backslash */
		else { preparseString[j] = string[i]; foundBS =0; j++; }
		i++;
	}
}
/* Function to replace the escape character in the token with their hex values */
void preparse(TokenizerT *tk, int numOfEscCharToken, int numOfEscCharDelims) {
	
	int size;
	size = strlen(tk -> tokenStream) - numOfEscCharToken + 1;
	/* Allocate memory to hold preparsed token (with hex values representation of escape characters) */
	tk -> pptokenStream = (char*)malloc(sizeof(char) * size);
	tk -> pptokenStream[size - 1] = '\0';
	/* Replace 2 byte escape characters in token */
	replaceEscChar(tk -> tokenStream, tk -> pptokenStream);
	size = strlen(tk -> delims) - numOfEscCharDelims + 1;
	/* Allocate memory to hold preparsed delimiters (with hex values representation of escape characters) */
	tk -> ppdelims = (char*)malloc(sizeof(char) * size);
	tk -> ppdelims[size - 1] = '\0';
	/* Replace 2 byte escape characters in delimiters */
	replaceEscChar(tk -> delims, tk -> ppdelims);
}

/* Function to extract tokens from the token stream */ 
char *TKGetNextToken(TokenizerT *tk) {
	
	int position, nextposition;
	char *newToken;

	/* Call skipDelimiterPos function to find first position of token */
	position = skipDelimiterPos(tk -> pptokenStream, tk -> ppdelims, tk -> position);
	if(tk -> pptokenStream[position] == '\0') {
		return NULL; 	/* Return null if the tokenstream has been fully traversed */
	}
	/* Get last position of token by called skipTokenPos function */
	nextposition = skipTokenPos(tk -> pptokenStream, tk -> ppdelims, position);
	tk -> position = nextposition;

	/* Extract token from the stream and store it into new character pointer newToken */
	newToken = createToken(tk -> pptokenStream, position, nextposition);

	/* Return the token */
	return newToken;
}

/* Main */
int main(int argc, char **argv) {
	
	char *token;
	int i, numOfEscCharToken, numOfEscCharDelims;
	
	/* Exit program if not enough arguments are inputted */
	if(argc < 3) {
		fprintf(stderr,"Did not enter enough arguments e.g. <Delimiters> <Token Stream> Program will exit.");
		return 0;
	}
	/* Exit program if too many arguments are inputted */
	else if(argc > 3) {
		fprintf(stderr,"Entered in too many arguments e.g. <Delimiters> <Token Stream> Program will exit.");
		return 0;
	}
	
	TokenizerT *tk = TKCreate(argv[1], argv[2]);	 	/* Call function to create tokenizer */ 
	numOfEscCharToken = findNumOfESC(tk -> tokenStream);  	/* Call function to find the number of escape characters in token stream */
	numOfEscCharDelims = findNumOfESC(tk -> delims);	/* Call function to find the number od escape characters in delimiter */
	preparse(tk, numOfEscCharToken, numOfEscCharDelims); 	/* Call function to replace potential 2 byte escape char to 1 byte 									representation */	
	
	/*Print out each token one by one on seperate lines */
	while(1) {
		token = TKGetNextToken(tk);
		if(token == NULL) 
			break;
		i = 0;
		while(token[i] != '\0') {
			/* If the character is one of the Escape Character then print out its hex value */
			if(token[i] == '\n' || token[i] == '\t' || token[i] == '\v' || token[i] == '\b' || token[i] == '\r' 					|| token[i] == '\f' || token[i] == '\a' || token[i] == '\\' || token[i] == '\"')
				printf("[0x%02x]",token[i]);
			else
				printf("%c", token[i]);
			i++;
		}
		printf("\n");
		free(token); /* Free the token after done printing */
	}

	printf("\n");
	TKDestroy(tk); /* Free the tokenizer structure */
	tk = NULL;
	return 0;
}
