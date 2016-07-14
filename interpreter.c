/* TODO:
   - Handle identifiers other than x and y. Q: These will be user-set
     parameters. How will they set it?

   - Avoid reading the string at each run? How?

 */

float expression();

char xtest[128] = "x*x+y";
char ytest[128] = "x-y";

char tmp[32];

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
  else {
    snprintf(tmp, 32, "%c", x);
    expected(tmp);
  }
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

float get_num() {
  if (!(is_digit(*look) || *look == '-')) {
    expected("float");
  }

  char buf[16];
  int i = 0;
  while (is_digit(*look) || *look == '.') {
    buf[i++] = *look;
    get_char();
  }
  buf[i] = 0;
  float ret = atof(buf);
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
  float sign = 1.0;

  if (*look == '-') {
    sign = -1;
  }

  if (*look == '(') {
    match('(');
    value = expression();
    match(')');
    return value;
  }
  else if (is_alpha(*look)) {
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
  return sign*value;
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
