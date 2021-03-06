// Client side C/C++ program to demonstrate Socket programming
//
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#define PORT 8080
#define MAXFILENAME 63

class clientContext
{
  public:
    bool withSSL = true;
    char cafile[MAXFILENAME+1] = "../certs/client/ca.crt";
    char cert[MAXFILENAME+1] = "../certs/client/client.crt";
    char key[MAXFILENAME+1] = "../certs/client/client.key";
};

SSL_CTX *create_ssl_context(clientContext *cliCtx)
{
  SSL_CTX *ctx;

  //ctx = SSL_CTX_new(TLS_server_method());
  ctx = SSL_CTX_new(SSLv23_client_method());
  if (!ctx)
    {
      perror("Unable to create SSL context");
      ERR_print_errors_fp(stderr);
      exit(EXIT_FAILURE);
    }

  SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);
  SSL_CTX_set_verify_depth(ctx, 5);

  if (!SSL_CTX_use_certificate_chain_file(ctx, cliCtx->cafile))
    {
      ERR_print_errors_fp(stderr);
      exit(EXIT_FAILURE);
    }
  SSL_CTX_load_verify_locations(ctx, cliCtx->cafile, NULL);

  if (SSL_CTX_use_certificate_file(ctx, cliCtx->cert, SSL_FILETYPE_PEM) <= 0)
    {
      ERR_print_errors_fp(stderr);
      exit(EXIT_FAILURE);
    }

  if (SSL_CTX_use_PrivateKey_file(ctx, cliCtx->key, SSL_FILETYPE_PEM) <= 0)
    {
      ERR_print_errors_fp(stderr);
      exit(EXIT_FAILURE);
    }

  return ctx;
}

int main(int argc, char const *argv[])
{
	int sock = 0, valread;
	struct sockaddr_in serv_addr;
  bool withSSL = true;
	char hello[] = "Hello from client via SSL";
	char buffer[1024] = {0};

  SSL_CTX *ctx = NULL;
  SSL *ssl = NULL;

  clientContext *cliCtx = new clientContext();

  //some overhead for the arguments
  for (int i=0; i<argc; ++i)
    {
      if (strcmp(argv[i],"-plain")==0)
        cliCtx->withSSL = false;
      else if ((strcmp(argv[i],"-cafile")==0) && (i+1 < argc) && (strlen(argv[i+1]) > 0))
        strncpy(cliCtx->cafile,argv[i+1],MAXFILENAME);
      else if ((strcmp(argv[i],"-cert")==0) && (i+1 < argc) && (strlen(argv[i+1]) > 0))
        strncpy(cliCtx->cert,argv[i+1],MAXFILENAME);
      else if ((strcmp(argv[i],"-key")==0) && (i+1 < argc) && (strlen(argv[i+1]) > 0))
        strncpy(cliCtx->key,argv[i+1],MAXFILENAME);
    }

  if (cliCtx->withSSL)
    {
      printf("create SSL context\n");

      ctx = create_ssl_context(cliCtx);
      ssl = SSL_new(ctx);
      if (ssl == NULL)
        ERR_print_errors_fp(stderr);
    }

  printf("create socket\n");

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		printf("\n socket creation error \n");
		return -1;
	}

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(PORT);

  printf("build address\n");
	// Convert IPv4 and IPv6 addresses from text to binary form
	if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0)
	{
		printf("\nInvalid address/ Address not supported \n");
		return -1;
	}

  printf("connect\n");
	if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{
		printf("\nConnection Failed \n");
		return -1;
	}

  if (cliCtx->withSSL)
    {
      if (!SSL_set_fd(ssl, sock))
        ERR_print_errors_fp(stderr);

      printf("SSL_connect\n");
      if (SSL_connect(ssl) < 0)
        ERR_print_errors_fp(stderr);

      // read server certificare
      X509 *cert = SSL_get_peer_certificate(ssl);
      if (cert == NULL)
        printf("Error: Could not get a certificate from: %s.\n", "");
      else
        printf("Retrieved the server's certificate from: %s.\n", "");

      X509_NAME *certname = X509_NAME_new();
      certname = X509_get_subject_name(cert);

      printf("Displaying the certificate subject data:\n");
      BIO *  bio  = BIO_new_fp(stdout, BIO_NOCLOSE);
      X509_NAME_print_ex(bio, certname, 0, 0);
      printf("\n");
    }

  if (cliCtx->withSSL)
    SSL_write(ssl , hello , strlen(hello)+1 );
  else
    send(sock , hello , strlen(hello)+1 , 0 );
	printf("Hello message sent\n");

  if (cliCtx->withSSL)
    valread = SSL_read( ssl , buffer, 1024);
  else
    read( sock , buffer, 1024);
	printf("out: %s\n",buffer);

  printf("end\n");

	return 0;
}
