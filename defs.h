
void checkrc(const char* s, int rc) {
  if (rc != 0)
    fprintf(stderr,"RC %s => %d\n", s, rc);
}

#define RC(x) checkrc(#x,x)
