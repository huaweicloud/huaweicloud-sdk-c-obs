
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __gnu_java_security_sig_rsa_EME_PKCS1_V1_5__
#define __gnu_java_security_sig_rsa_EME_PKCS1_V1_5__

#pragma interface

#include <java/lang/Object.h>
#include <gcj/array.h>

extern "Java"
{
  namespace gnu
  {
    namespace java
    {
      namespace security
      {
        namespace prng
        {
            class IRandom;
        }
        namespace sig
        {
          namespace rsa
          {
              class EME_PKCS1_V1_5;
          }
        }
        namespace util
        {
            class PRNG;
        }
      }
    }
  }
  namespace java
  {
    namespace security
    {
      namespace interfaces
      {
          class RSAKey;
      }
    }
  }
}

class gnu::java::security::sig::rsa::EME_PKCS1_V1_5 : public ::java::lang::Object
{

  EME_PKCS1_V1_5(jint);
public:
  static ::gnu::java::security::sig::rsa::EME_PKCS1_V1_5 * getInstance(jint);
  static ::gnu::java::security::sig::rsa::EME_PKCS1_V1_5 * getInstance(::java::security::interfaces::RSAKey *);
  virtual JArray< jbyte > * encode(JArray< jbyte > *);
  virtual JArray< jbyte > * encode(JArray< jbyte > *, ::gnu::java::security::prng::IRandom *);
  virtual JArray< jbyte > * encode(JArray< jbyte > *, ::java::util::Random *);
  virtual JArray< jbyte > * decode(JArray< jbyte > *);
private:
  JArray< jbyte > * assembleEM(JArray< jbyte > *, JArray< jbyte > *);
  jint __attribute__((aligned(__alignof__( ::java::lang::Object)))) k;
  ::java::io::ByteArrayOutputStream * baos;
  ::gnu::java::security::util::PRNG * prng;
public:
  static ::java::lang::Class class$;
};

#endif // __gnu_java_security_sig_rsa_EME_PKCS1_V1_5__
