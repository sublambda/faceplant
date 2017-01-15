
void checkrc(const char* s, int rc) {
  if (1 || rc != 0) {
    fprintf(stderr,"RC %s => %d\n", s, rc);
    fflush(stderr);
  }
}
#define RC(x) checkrc(#x,x)
