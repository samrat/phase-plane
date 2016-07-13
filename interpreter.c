/* TODO:
   - Handle floating point constants

   - Handle identifiers other than x and y. Q: These will be user-set
     parameters. How will they set it?

   - Avoid reading the string at each run? How?
 */


char xtest[128] = "x*x+y";
char ytest[128] = "x-y";

char *look = &xtest[0];

struct {
  float x,y;
} env;

void get_char() {
  look++;
}

void abort() {
  exit(1);
}

void expected(char *s) {
  printf("%s expected\n", s);
  abort();
}

void match(char x) {
  if (*look == x) {
    get_char();
  }
  else expected("FIXME");
}

bool is_alpha(char c) {
  if ((c >= 'a' && c <= 'z') ||
      (c >= 'A' && c <= 'Z'))
    return true;
  else
    return false;
}

bool is_digit(char c) {
  if (c >= '0' && c <= '9')
    return true;
  return false;
}

bool is_addop(char c) {
  if (c == '+' || c == '-')
    return true;
  return false;
}

int get_num() {
  if (!is_digit(*look)) {
    expected("integer");
  }
  int ret = *look - '0';
  get_char();
  return ret;
}

char get_name() {
  if (!is_alpha(*look))
    expected("name");
  char ret = *look;
  get_char();
  return ret;
}

float factor() {
  float value;
  /* TODO other cases(paren-ed exprs, idents) */
  if (is_alpha(*look)) {
    switch (*look) {
    case 'x': {
      value = env.x;
    } break;
    case 'y': {
      value = env.y;
    } break;
    }
    get_char();
  }
  else {
    value = get_num();
  }
  return value;
}

float term() {
  float value = factor();
  while ((*look == '*') || (*look == '/')) {
    switch (*look) {
    case '*': {
      match('*');
      value = value * factor();
    } break;
    case '/': {
      match('/');
      value = value / factor();
    } break;
    }
  }

  return value;
}

float expression() {
  float value;

  value = term();
  while (is_addop(*look)) {
    switch(*look) {
    case '+': {
      match('+');
      value = value + term();
    } break;
    case '-': {
      match('-');
      value = value - term();
    } break;
    }
  }

  return value;
}
