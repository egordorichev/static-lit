#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#include <lit.h>

void show_repl() {
	printf("FIXME: implement REPL!\nIf you want to run a file, use 'lit file.lit'\nUse 'lit -h' for help\n");
	// This is super old

	/*
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

	// TODO: quit command
	lit_free_vm(&vm);*/
}

void show_help() {
	printf("lit - powerful and fast static-typed language\n");
	printf("\tlit [file]\tRun the file\n");
	printf("\t-e --exec [code string]\tExecutes a string of code\n");
	printf("\t-h --help\tShows this hint\n");
}

static char* read_file(const char* path) {
	FILE* file = fopen(path, "rb");

	if (file == NULL) {
		fprintf(stderr, "Could not open file \"%s\"\n", path);
		exit(74);
	}

	fseek(file, 0L, SEEK_END);
	size_t fileSize = (size_t) ftell(file);
	rewind(file);

	char* buffer = (char*) malloc(fileSize + 1);

	if (buffer == NULL) {
		fprintf(stderr, "Not enough memory to read \"%s\"\n", path);
		exit(74);
	}

	size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);

	if (bytesRead < fileSize) {
		fprintf(stderr, "Could not read file \"%s\"\n", path);
		exit(74);
	}

	buffer[bytesRead] = '\0';
	fclose(file);

	return buffer;
}

int main(int argc, char** argv) {
  if (argc == 1) {
  	show_repl();
  } else {
	  for (int i = 1; i < argc; i++) {
		  char* arg = argv[i];

		  if (arg[0] == '-') {
			  if (strcmp(arg, "-e") == 0 || strcmp(arg, "--exec") == 0) {
				  if (i == argc - 1) {
					  printf("Usage: lit -e [code]");
				  } else {
					  return lit_eval(argv[i + 1]) ? 0 : 2;
				  }
			  } else if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0) {
					show_help();
			  } else {
			  	printf("Unknown option %s! Run with -h for help.", arg);
			  	return -1;
			  }
		  } else {
			  const char* source_code = read_file(arg);
			  bool had_error = !lit_eval(source_code);
			  free((void*) source_code);

			  return had_error ? 2 : 0;
		  }
	  }
  }

  return 0;
}