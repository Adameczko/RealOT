package main

import (
	"crypto/x509"
	"encoding/pem"
	"fmt"
	"log"
	"os"
)

func main() {
	keyFile := "key.pem"
	if len(os.Args) > 1 {
		keyFile = os.Args[1]
	}

	keyData, err := os.ReadFile(keyFile)
	if err != nil {
		log.Panicf("failed to read key \"%v\": %v", keyFile, err)
	}

	keyBlock, _ := pem.Decode(keyData)
	if keyBlock == nil {
		log.Panicf("key \"%v\" is empty")
	}

	privateKey, err := x509.ParsePKCS1PrivateKey(keyBlock.Bytes)
	if err != nil {
		log.Panic("failed to read PKCS1 RSA private key: %v", err)
	}

	fmt.Println("N", privateKey.N)
	fmt.Println("E", privateKey.E)
}
