#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#include "lit.h"
#include "lit_debug.h"

void show_repl() {
	LitVm vm;
	lit_init_vm(&vm);

	char line[1024];

	for (;;) {
		printf("> ");

		if (!fgets(line, sizeof(line), stdin)) {
			printf("\n");
			break;
		}

		lit_execute(&vm, line);
	}

	// todo: quit command
	lit_free_vm(&vm);
}

static char* read_file(const char* path) {
	FILE* file = fopen(path, "rb");

	if (file == NULL) {
		fprintf(stderr, "Could not open file \"%s\".\n", path);
		exit(74);
	}

	fseek(file, 0L, SEEK_END);
	size_t fileSize = ftell(file);
	rewind(file);

	char* buffer = (char*) malloc(fileSize + 1);

	if (buffer == NULL) {
		fprintf(stderr, "Not enough memory to read \"%s\".\n", path);
		exit(74);
	}

	size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);

	if (bytesRead < fileSize) {
		fprintf(stderr, "Could not read file \"%s\".\n", path);
		exit(74);
	}

	buffer[bytesRead] = '\0';
	fclose(file);

	return buffer;
}

int main(int argc, char** argv) {
  /*if (argc == 1) {
  	show_repl();
  } else {
	  for (int i = 1; i < argc; i++) {
		  char* arg = argv[i];

		  if (arg[0] == '-') {
			  if (strcmp(arg, "-e") == 0 || strcmp(arg, "--exec") == 0) {
				  if (i == argc - 1) {
					  printf("Usage: lit -e [code]");
				  } else {
					  i++;

					  LitVm vm;
					  lit_init_vm(&vm);
					  LitInterpretResult result = lit_execute(&vm, argv[i]);
					  lit_free_vm(&vm);
					  return result == INTERPRET_OK ? 0 : -2;
				  }
			  } else if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0) {
				  printf("lit - simple and fast scripting language\n");
					printf("\tlit [file]\tRun the file\n");
				  printf("\t-e --exec [code string]\tExecutes a string of code\n");
				  printf("\t-h --help\tShows this hint\n");
			  } else {
			  	printf("Unknown option %s! Run with -h for help.", arg);
			  	return -1;
			  }
		  } else {
			  LitVm vm;
			  lit_init_vm(&vm);
			  LitInterpretResult result = lit_execute(&vm, read_file(arg));
			  lit_free_vm(&vm);
			  return result == INTERPRET_OK ? 0 : -2;
		  }
	  }
  }

  return 0;*/

	LitVm vm;
	lit_init_vm(&vm);
	LitInterpretResult result = lit_execute(&vm, "print 10");
	lit_free_vm(&vm);
	return result == INTERPRET_OK ? 0 : -2;
}