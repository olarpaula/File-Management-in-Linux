/**
Formatul apelurilor:
./t1 histogram bits =<number_of_bits> path=<file_path>
./t1 runs path=<file_path>
./t1 template bits=<number_of_bits> template=<number_of_template> t_path=<template_file_path> path=<file_path>
./t1 template bits=<number_of_bits> t_path=<t_path=<template_file_path> path=<file_path>
./t1 list <filtering_options> path=<dir_path>
./t1 list recursive <filtering_options> path=<dir_path>
*/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#define MAX_PATH 1024
#define MAX 1024

int printOnce = 1;

/**
    Functii secundare utilizate in rezolvarea histogramei pe n biti
    Prin functia generateSubsets returnez un tablou format din toate combinatiile de 0 si 1
     din n biti, fiecare combinatie fiind generata de functia toBinArray
*/

void toBinArr(int n, int x, char* strRes) {
	int j = 0;
	for(int i = n-1; i >= 0; i--) {
		int k = x >> i;

		strRes[j] = (k & 1) ? '1' : '0';
		j++;
	}
	strRes[n] = 0;
}

char** generateSubsets (int n, int nrElements) {
	char string[n+1];
	char** allSubsets = (char**)malloc(nrElements * sizeof(char*));

	for (int i = 0; i < nrElements; i++) {
		toBinArr(n, i, string);
		allSubsets[i] = malloc(strlen(string) + 1);
		strcpy(allSubsets[i], string);
	}

	return allSubsets;
}

/**
    Functia principala utilizata in rezolvarea histogramei pentru n biti
    n - numarul de biti dupa care se calculeaza histograma
    path - calea catre fisierul utilizat
    keepTrack - vectorul de frecventa pentru contorizarea aparitiilor valorilor pe n biti
    showFrequency - parametru prin care decidem cum apelam functia. pentru valoarea 1, functia este apelata pentru calcularea si afisarea histogramei,
     iar altfel, functia este apelata in cadrul problemei de listare a fisierelor, in care ne intereseaza doar vectorul keepTrack
    Abordare - la fiecare buffer citit, un octet e impartit in subsiruri de n biti si se contorizeaza aparitia fiecaruia
*/

void histogram(int n, char* path, int* keepTrack, int showFrequency) {
	int fd;
	int bytesRead;
	int nrElements = pow(2, n);
	char** allSubsets = generateSubsets(n, nrElements);
	unsigned char buf[MAX];
	//int* keepTrack = calloc(nrElements, sizeof(int));

	if (n != 1 && n != 2 && n != 4 && n != 8) {
		printf("ERROR\nInvalid number of bits\nSupported values are: 1, 2, 4, 8\n");
		exit(1);
	}

	if ((fd = open(path, O_RDONLY)) < 0) {
		printf("ERROR\nInvalid file path\n");
		exit(2);
	}

	while ((bytesRead = read(fd, buf, sizeof(buf))) > 0) {
		char subCompare[n+1];

		for(int k = 0; k < MAX; k++) {
		int shift = 7;
			for(int count = 0; count < 8; count += n) {
				for (int i = 0; i < n; i++) {
					subCompare[i] = ((buf[k] >> shift) & 1) + '0';
					shift--;
				}

				subCompare[n] = 0;

				for (int i = 0; i < nrElements; i++) {
					if (strcmp(allSubsets[i], subCompare) == 0) {
						keepTrack[i]++;
					}
				}
			}
		}
	}

	close(fd);

	if (showFrequency == 1) {
	printf("SUCCESS\n");
	for(int i = 0; i < nrElements; i++)
		printf("%s: %d\n",allSubsets[i], keepTrack[i]);
	}

	for(int i = 0; i < nrElements; i++)
		free(allSubsets[i]);
	free(allSubsets);
}

/**
    Functia principala pentru calcularea celei mai lungi subsecvente consecutive de 1
    path - calea catre fisier
    Abordare - la fiecare iteratie prin bytes se retine posibila adresa de inceput a celei mai lungi secvente si se contorizeaza mereu lungimea secventelor de 1
*/
void runs (char* path)
{
	int fd;
	int bytesRead;
	unsigned char buf[MAX];
	int bytesCount = 0; //la ce byte am ajuns in fisier

	int currLength = 0;
	int maxLength = 0;


	int doKeep = 1; //daca numar iar un sir nou de 1 stochez byteul de la care incep numaratoarea

	int offsetByte = 0; //la ce byte suntem la prima adresa a celui mai lung subsir de 1
	int offsetBit = 0;
	int currByte; //byte ul de la care poate incepe un nou subsir de 1 care nu e mai mare ca maximul
	int currBit;

	if ((fd = open(path, O_RDONLY)) < 0) {
		printf("ERROR\nInvalid file path\n");
		exit(1);
	}

	while ((bytesRead = read(fd, buf, sizeof(buf))) > 0) {
		for(int k = 0; k < MAX; k++) {
			for (int shift = 7; shift >= 0; shift--) {
				if (1 & (buf[k] >> shift)) {
					currLength++;

					if (doKeep == 1) {
						currByte = bytesCount;
						currBit = 7 - shift;
						doKeep = 0;
					}
				}
				else {
					if (currLength > maxLength) {
						maxLength = currLength;
						offsetByte = currByte;
						offsetBit = currBit;
					}

					currLength = 0;

					//permit o noua stocare a indexului byteului
					doKeep = 1;
				}
			}

		bytesCount++;
		}
	}

	close(fd);

	printf("SUCCESS\nLength of the longest run: %d\nOffset: %d bytes + %d bits\n", maxLength, offsetByte, offsetBit);
}

/**
    Structurile utilizate in rezolvarea problemei privind cautarea sabloanelor si listarea fisierelor cu filtrul template=ok
*/

typedef struct metaData
{
	unsigned short version;
	unsigned short no_of_template_headers;  //cate categorii de sabloane
}metaData;

typedef struct templateHeader
{
	unsigned short int size_of_template;
	unsigned short int no_of_templates; //cate sabloane intr-o categorie
	unsigned int offset;
}templateHeader;

/**
    Functia isOkTemplate este folosita pentru verificarea corectitudinii fisierelor in conditia de a avea structura de fisiere cu continut de templateuri
    path - calea catre fisierul utilizat
    showMessage - parametru care indica in cadrul carei probleme este apelata functia suport
      Pentru showMessage=1, functia este folosita in calcularea sabloanelor de biti si in caz de eroare la verificarea fisierului, se afiseaza eroarea, altfel
      este folosita pentru verificarea in fisierelor in functia de listare si nu se doreste decat valoare de adevarat sau fals pentru conditia template=ok
*/

int isOkTemplate(char* path, int showMessage)
{
	int fd;
	int bytesRead;

	metaData metaD = {0};
	templateHeader templateH = {0};

	if ((fd = open(path, O_RDONLY)) < 0) {
		if (showMessage == 1) {
			printf("ERROR\nInvalid template path\n");
		}
		return 0;
	}

	if ((bytesRead = read(fd, &metaD, sizeof(metaData))) < 0) {
		if (showMessage == 1) {
			printf("ERROR\nWRong metadata");
		}
		return 0;
	}

	if (metaD.version < 12345 || metaD.version > 54321) {
		if (showMessage == 1) {
			printf("ERROR\nWrong version\n");
		}
		return 0;
	}

	if (metaD.no_of_template_headers < 0) {
		if (showMessage == 1) {
			printf("ERROR\nWrong metadata");
		}
		return 0;
	}

	templateHeader* tH = malloc(metaD.no_of_template_headers * sizeof(templateHeader));

	for(int i = 0; i < metaD.no_of_template_headers; i++) {
		if ((bytesRead = read(fd, &templateH, sizeof(templateHeader))) < 0) {
			if (showMessage == 1) {
				printf("ERROR\nWrong metadata\n");
			}
			return 0;
		}
		else
			tH[i] = templateH;
	}

	for (int i = 0; i < metaD.no_of_template_headers; i++) {
		if (tH[i].size_of_template != 8 && tH[i].size_of_template != 16 && tH[i].size_of_template != 24 && tH[i].size_of_template != 32) {
			if (showMessage == 1) {
				printf("Wrong metadata\n");
			}
			return 0;
		}
	}

	for (int i = 0; i < metaD.no_of_template_headers; i++) {
		for (int j = 0; j < metaD.no_of_template_headers; j++) {
			if (i != j && tH[i].offset < tH[j].offset + tH[j].size_of_template * tH[j].no_of_templates / 8 && tH[j].offset < tH[i].offset + tH[i].size_of_template * tH[i].no_of_templates / 8) {
				if (showMessage == 1) {
					printf("Overlaying categories\n");
				}
				return 0;
			}
		}
	}

	return 1;
}

/**
        Functie suport in rezolvarea problemei 3, de cautare de sabloane. Functia returneaza numarul de aparitii a unui substring intr-un string
*/
int str(char* string, char* allTemplates)
{
	int count1 = 0;

	while ((string = strstr(string, allTemplates))) {
		string++;
		count1++;
	}

	return count1;
}

/**
    Functia principala pentru rezolvarea cautarii sabloanelor de biti
    bits - numarul de biti pe baza carora se cauta o anumita categorie
    nrOfTemplate - numarul pentru cautarea unui template anume
      Pentru nrOfTemplate > 0, se cauta o valoare de template specificata, altfel, se cauta toate template-urile din categoria bits
    templateName - calea catre fisierul template
    fileName - calea catre fisierul cu secvente de numere aleatoare
    Abordare - Se face initial o singura citire utilizand un bufer, in urma careia se extrag ultimii n biti care urmeaza a fi concatenati in fata
      urmatorului bufer citit. In continuare pentru fiecare citire se realizeaza aceleasi extrageri si concatenari. La fiecare citire se utilizeaza functia str
      pentru a calcula numarul de aparitii. Dupa prima citire, contorizarea incepe de la valoarea celui de al doilea bit pentru a evita posibilele
      contorizari de 2 ori
*/

void template (int bits, int nrOfTemplate, char* templateName, char* fileName)
{
	metaData metaD = {0};
	templateHeader templateH = {0};

	int bytesRead;
	int fd;
	int fd2;

	int startOffsetIdx; //unde sa ma pozitionez in fisier
	int numberTempl; //cate template uri sunt in categoria cautata
	int foundCategory = -1; //daca cumva numarul de template cautat nu exista

	int verify = isOkTemplate(templateName, 1);
	if(verify == 0) {
		exit(1);
	}

	if(bits != 8 && bits != 16 && bits != 32 && bits != 24) {
		printf("ERROR\nInvalid template category");
		exit(1);
	}


	fd = open(templateName, O_RDONLY);

	bytesRead = read(fd, &metaD, sizeof(metaData));

	//printf("\n%d\n", metaD.version);
	//printf("\n%d\n", metaD.no_of_template_headers);

	for(int i = 1; i <= metaD.no_of_template_headers; i++) {
		bytesRead = read(fd, &templateH, sizeof(templateHeader));

		//printf("categoria %d are sabloane de dim - %d - %d sabloane incepand de la offsetul %d\n", i, templateH.size_of_template, templateH.no_of_templates, templateH.offset);

		if (bits == templateH.size_of_template) {
			startOffsetIdx = templateH.offset;
			numberTempl = templateH.no_of_templates;
			foundCategory = 1;
		}
	}

	if (foundCategory < 0 || numberTempl < nrOfTemplate) {
		printf("ERROR\nInvalid template category\n");
		exit(6);
	}

	//if(numberTempl < nrOfTemplate) {
	//	printf("ERROR\nInvalid template number\n");
	//	exit(7);
	//}

	lseek(fd, startOffsetIdx, SEEK_SET);

	int howManyBytes = bits/8;
	unsigned char buf[howManyBytes];

	char** allTemplates = (char**)malloc(numberTempl * sizeof(char*));

	//extrag toate template urile din categoria cautata pentru a verifica daca sunt citite corect si le stochez intr un tablou
	for(int i = 0; i < numberTempl; i++) {
		if ((bytesRead = read(fd, buf, sizeof(buf))) < 0) {
			printf("ERROR\nWrong metadata\n");
			exit(8);
		}

		char subCompare[bits+1];
		int j = 0;

		for (int k = 0; k < howManyBytes; k++) {
			int shift = 7;
			while (shift >= 0) {
				subCompare[j] = ((buf[k] >> shift) & 1) + '0';
				shift--;
				j++;
			}
		}

		subCompare[bits] = 0;

		allTemplates[i] = malloc(strlen(subCompare) + 1);
		strcpy(allTemplates[i], subCompare);
	}

	close(fd);

	//for(int i = 0; i < numberTempl; i++)
	//{
	//	printf("%s\n", allTemplates[i]);
	//}


	if ((fd2 = open(fileName, O_RDONLY)) < 0) {
		printf("ERROR\nInvalid file path!\n");
		exit(9);
	}

	unsigned char buf2[MAX];
	int count1 = 0;	//numarul de aparitii pentru un template anume idntr-o categorie
	int* aparitii = calloc(numberTempl, sizeof(int)); //numarul de aparitii pentru toate template-urile dintr-o categorie


	char* adauga = calloc(bits+1, sizeof(char));

	if ((bytesRead = read(fd2, buf2, sizeof(buf2))) > 0) {
		char* string = malloc((8*MAX+1)*sizeof(char));
		int idx = 0;

		for(int k = 0; k < MAX; k++) {
			for (int shift = 7; shift >= 0; shift--) {
				string[idx] = ((buf2[k] >> shift) & 1) + '0';
				idx++;

			}
		}

		string[8*MAX] = 0;

		if (nrOfTemplate > 0) {
			count1 += str(string, allTemplates[nrOfTemplate-1]);
		}

		else {
			for(int i = 0; i < numberTempl; i++) {
				aparitii[i] += str(string, allTemplates[i]);
			}
		}

		int l = 0;

		for (int i = 0; i < bits; i++) {
			adauga[l] = string[8*MAX-1-bits+1+i];
			l++;
		}
	}

	while ((bytesRead = read(fd2, buf2, sizeof(buf2))) > 0) {
		char* string = malloc((8*MAX+1+bits)*sizeof(char));
		int idx = 0;

		for(int i = 0; i < bits; i++) {
			string[idx] = adauga[i];
			idx++;
		}

		for(int k = 0; k < MAX; k++) {
			for (int shift = 7; shift >= 0; shift--) {
				string[idx] = ((buf2[k] >> shift) & 1) + '0';
				idx++;
			}
		}

		string[8*MAX+bits] = 0;

		if (nrOfTemplate > 0) {
			count1 += str(string+1, allTemplates[nrOfTemplate-1]);
		}

		else {
			for(int i = 0; i < numberTempl; i++) {
				aparitii[i] += str(string+1, allTemplates[i]);
			}
		}

		int l = 0;

		for (int i = 0; i < bits; i++) {
			adauga[l] = string[8*MAX+i];
			l++;
		}

		adauga[bits] = 0;
	}

	close(fd2);


	printf("SUCCES!\nNumber of templates in category: %d\n", numberTempl);

	if (nrOfTemplate > 0) {
		printf("Occurences of template %2d of length %d: %d\n", nrOfTemplate, bits, count1);
	}

	else {
		for(int i = 0; i < numberTempl; i++) {
			printf("Occurences of template %2d of length %d: %d\n", i+1, bits, aparitii[i]);
		}
	}

	for(int i = 0; i < numberTempl; i++)
		free(allTemplates[i]);
	free(allTemplates);

	free(aparitii);
}

/*functia isOkHistogram e folosita pentru a calcula procentul posibil de eroare de 1% si e apelata de functia list*/
/**
    Functia isOkHistogram este functie suport apelata din cadrul functiei de listare si returneaza o valoare de adevarat sau fals pentru verificarea
    diferentei numarului de aparitii a fiecarei combinatii posibile a vectorului de frecventa, returnand un rezultat pozitiv pentru o diferenta de maxim 1%
    nrElements - numarul de elemente al vectorului de frecventa utilizat in calcularea histogramei
    keepTrack - vectorul de frecventa
*/
int isOkHistogram(int nrElements, int* keepTrack)
{
	int sum = 0;
	for(int i = 0; i < nrElements; i++) {
		sum += keepTrack[i];
	}

	if(sum == 0)
		return 0;

	for(int i = 0; i < nrElements; i++) {
		for(int j = i+1; j < nrElements; j++) {
			if( abs(keepTrack[i] - keepTrack[j]) > sum/100) {
				return 0;
			}
		}
	}

	return 1;
}

/**
    Functie suport pentru functia de listare prin care se verifica corectitudinea argumentelor introduse
*/
int checkFilter(char* filter)
{
	if(strncmp("name_contains=", filter, 14) != 0 && strncmp(filter, "hist_random=", 12) != 0 && strncmp(filter, "size_greater=", 13) != 0
&& strcmp(filter, "template=ok") != 0)
		return 0;
	return 1;
}

/**
    Functia de listare a fisierelor/directoarelor care respecta un anumit filtru
    recursive - parametru care indica daca se realizeaza cautarea strict in directorul specificat (recursive=0) sau in toata ierarhia acestuia (recursive=1)
    filter - filtru pe baza caruia se face cautarea de fisiere/directoare
    path - calea catre directorul din care se incepe cautarea
    Abordare - Am construit pentru fiecare intrare noua path-ul catre fisierele/directoarele accesate si pe baza verificarii conditiilor specifice fiecarui filtru
    se decide afisarea sau nu a acestor path-uri
*/

void list(int recursive, char* filter, char* path)
{
	DIR* dir = opendir(path);
   	struct dirent* d;

	if (dir == NULL) {
		printf("\nInvalid directory path\n");
		exit(1);
	}

	int chkFilter = checkFilter(filter);
	if(!chkFilter) {
		printf("ERROR\nInvalid filter!\nTry\ntemplate=ok\nhist_random=<N>\nname_contains=<string>\nsize_greater=<value>\n");
			exit(2);
	}

	if (printOnce == 1) {
		printf("SUCCESS\n");
		printOnce = 0;
	}

    	while ((d = readdir(dir)) != NULL) {
		char new_path[MAX_PATH] = {0};
		 if (d->d_type == DT_DIR) {
		 	if (!strcmp(d->d_name, "..") || !strcmp(d->d_name, ".")) {
		    		continue;
			}

			int append = snprintf(new_path, MAX_PATH, "%s/%s", path, d->d_name);

			if (append && recursive == 1) {

		    		list(recursive, filter, new_path);
			}

	    	}

		if (strncmp("name_contains=", filter, 14) == 0) {
			char* searchedString = malloc((strlen(filter) - 14) * sizeof(char));
			strcpy(searchedString, filter + 14);

			char new[MAX] = {0};
			int append = snprintf(new, MAX_PATH, "%s/%s", path, d->d_name);
			if (append) {
				if (strstr(d->d_name, searchedString)) {
					printf("%s\n", new);
	     			}
			}
		}

		else if (strncmp(filter, "hist_random=", 12) == 0) {
			if (d->d_type == DT_REG) {
				char new[MAX] = {0};
				int append = snprintf(new, MAX_PATH, "%s/%s", path, d->d_name);
				if (append) {
					int bits;
					sscanf(filter, "hist_random=%d", &bits);
					if (bits != 1 && bits != 2 && bits != 4 && bits != 8) {
						printf("ERROR\nInvalid number of bits\nSupported values are: 1, 2, 4, 8\n");
						exit(1);
					}

					int nrElements = pow(2, bits);
					int keepTrack[nrElements];
					for (int i = 0; i < nrElements; i++)
						keepTrack[i] = 0;

					histogram(bits, new, keepTrack, 0);

					int checkHist = isOkHistogram(nrElements, keepTrack);

					if (checkHist == 1) {
						printf("%s\n", new);
					}
				}
			}
		}

		else if (strncmp(filter, "size_greater=", 13) == 0) {
			char new[MAX] = {0};
			int append = snprintf(new, MAX_PATH, "%s/%s", path, d->d_name);
			if (append) {
				if (d->d_type == DT_REG) {
					struct stat stStruct;
					int value;
					sscanf(filter, "size_greater=%d", &value);

					stat(new, &stStruct);
					int blkSize = stStruct.st_size;
					if (value < blkSize) {
						printf("%s\n", new);
					}
				}
			}
		}

		else if (strcmp(filter, "template=ok") == 0) {
			char new[MAX] = {0};
			int append = snprintf(new, MAX_PATH, "%s/%s", path, d->d_name);
			if (append) {
				if (d->d_type == DT_REG) {
					int isOkTempl = isOkTemplate(new, -1);

					if (isOkTempl == 1) {
						printf("%s\n", new);
					}
				}
			}
		}
	}

    closedir(dir);
}

/**
    Functie suport pentru verificarea corectitudinii introducerii argumentelor in linia de comanda
*/
int wrongParam(char* argv, char* param, int n)
{
	if (strncmp(argv, param, n) != 0)
		return 1;
	return 0;
}

int main (int argc, char** argv)
{
	if (strcmp(argv[1], "histogram") == 0) {
		if (argc != 4) {
			fprintf(stderr, "usage: %s histogram bits=<number_of_bits> path=<file path>\n", argv[0]);
			exit(1);
		}

		if (wrongParam(argv[2], "bits=", 5) || wrongParam(argv[3], "path=", 5)) {
			fprintf(stderr, "usage: %s histogram bits=<number_of_bits> path=<file path>\n", argv[0]);
			exit(1);
		}

		int bits;
		if (sscanf(argv[2], "bits=%d", &bits) < 0) {
			printf("ERROR\nBits reading error!\n");
			exit(2);
		}

		char* path = malloc((strlen(argv[3]) - 5) * sizeof(char));
		strcpy(path, argv[3] + 5);

		int nrElements = pow(2, bits);
		int keepTrack[nrElements];
		for (int i = 0; i < nrElements; i++)
			keepTrack[i] = 0;
		histogram(bits, path, keepTrack, 1);

	}

	else if (strcmp(argv[1], "runs") == 0) {
		if (argc != 3) {
			fprintf(stderr, "usage: %s runs path=<file path>\n", argv[0]);
			exit(1);
		}

		if (wrongParam(argv[2], "path=", 5) != 0) {
			fprintf(stderr, "usage: %s runs path=<file path>\n", argv[0]);
			exit(1);
		}

		char* path = malloc((strlen(argv[2]) - 5) * sizeof(char));
		strcpy(path, argv[2] + 5);

		runs(path);
	}

	else if (strcmp(argv[1], "template") == 0) {
		if (argc == 5) {
			if (wrongParam(argv[2], "bits=", 5) || wrongParam(argv[3], "t_path=", 7) || wrongParam(argv[4], "path=", 5)) {
				fprintf(stderr, "Usage: %s template bits=<number_of_bits> [template=<number_of_template>] t_path=<template_file_path> path=<file_path>\n", argv[0]);
				exit(1);
			}

			int bits; // = atoi(argv[2]);
			if (sscanf(argv[2], "bits=%d", &bits) < 0) {
				printf("ERROR\nBits reading error!\n");
				exit(2);
			}

			char* t_path = malloc((strlen(argv[3]) - 7) * sizeof(char));
			strcpy(t_path, argv[3] + 7);

			char* path = malloc((strlen(argv[4]) - 5) * sizeof(char));
			strcpy(path, argv[4] + 5);

			template(bits, -1, t_path, path);
		}

		else if (argc == 6) {
			int bits;// = atoi(argv[2]);

			if (wrongParam(argv[2], "bits=", 5) || wrongParam(argv[3], "template=", 9) || wrongParam(argv[4], "t_path=", 7) || wrongParam(argv[5], "path=", 5)) {
				fprintf(stderr, "Usage: %s template bits=<number_of_bits> [template=<number_of_template>] t_path=<template_file_path> path=<file_path>\n", argv[0]);
				exit(1);
			}

			if (sscanf(argv[2], "bits=%d", &bits) < 0) {
				printf("ERROR\nReading error!\n");
				exit(2);
			}

			int template_nr;// = atoi(argv[3]);
			if (sscanf(argv[3], "template=%d", &template_nr) < 0) {
				printf("ERROR\nTemplate number reading error!\n");
				exit(2);
			}

			char* t_path = malloc((strlen(argv[4]) - 7) * sizeof(char));
			strcpy(t_path, argv[4] + 7);

			char* path = malloc((strlen(argv[5]) - 5) * sizeof(char));
			strcpy(path, argv[5] + 5);

			template(bits, template_nr, t_path, path);
		}

		else {
        		fprintf(stderr, "Usage: %s template bits=<number_of_bits> [template=<number_of_template>] t_path=<template_file_path> path=<file_path>\n", argv[0]);
        		exit(1);
    		}
	}

	else if(strcmp(argv[1], "list") == 0) {
		if (argc == 4) { ///t1 list [recursive] <filtering_options> path=<dir_path>
			if (wrongParam(argv[3], "path=", 5)) {
        			fprintf(stderr, "Usage: %s list [recursive] <filtering_options> path=<dir_path>\n", argv[0]);
				exit(1);
			}

			char* path = malloc((strlen(argv[3]) - 5) * sizeof(char));
			strcpy(path, argv[3] + 5);

			list(0, argv[2], path);
		}

		else if (argc == 5) {
			if (wrongParam(argv[2], "recursive", 9) || wrongParam(argv[4], "path=", 5)) {
        			fprintf(stderr, "Usage: %s list [recursive] <filtering_options> path=<dir_path>\n", argv[0]);
				exit(1);
			}

			char* path = malloc((strlen(argv[4]) - 5) * sizeof(char));
			strcpy(path, argv[4] + 5);

			list(1, argv[3], path);
		}

		else {
        		fprintf(stderr, "Usage: %s list [recursive] <filtering_options> path=<dir_path>\n", argv[0]);
        		exit(1);
    		}
	}

	else {
		printf("ERROR!\nWrong command entry!\nFunction not found!\nYou can try:\nhistogram\nruns\ntemplate\nlist\n");
		exit(3);
	}

	exit(0);

}
