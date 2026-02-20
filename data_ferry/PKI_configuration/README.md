# Data Ferry PKI Infastructure Secure Comms Configuration

1. [Raspbian OS Installation](/data_ferry/system_install/README.md)
2. [Software and Dependencies Installation](/data_ferry/software_installation/README.md)
3. [Network Configuration and Component Servers](/data_ferry/network_configuration/README.md)
4. [PKI_configuration](/data_ferry/PKI_configuration/README.md)  <--You are here
5. [Data Ferry Usage, Server Management, and Debugging](/data_ferry/server_management/README.md) 

In this section we will create a Public Key Infastructure (PKI) configuration that will mutually authenticate and encrypt comms between our FoxNodes and the Drone Server and vice versa.

We will mostly work within the Drone Server Raspbian OS; however, we will have to "load" the generated certificates onto each FoxNode independently. This is also discussed in this section.

**NOTE:** This is only used to create secure HTTP/TLS sessions for encrypted HTTP data. Since the Wi-Fi network is considered a "controlled" private network, we assume all devices that connect to the network are trusted and that the Wi-Fi protocol privides lower level secure communications and device authentication.

**WARNING:** For all of the steps below, we use the NIST OU configuration for certificate subject creation. If you want to chage these to your specific OU, you will need to edit each of the command line command and/or configuration files to match your OU. It is permissible to use the "NIST" configuration below for testing purposes only, but you will need to change it if you plan to use this in a production network.
	
## Generate CA, Server certificates, and FoxNode client certificates

First we must make our Drone Server the Certificate Authority since we are "self-signing" our certificates.

### Step 1: Prepare a working folder on the Drone Server (For all of our certificates)
```
mkdir -p ~/mtls/{ca,server,clients}
cd ~/mtls
```

### Step 2: Create a Certificate Authority (CA)
```
openssl genrsa -out ca/ca.key 4096
chmod 600 ca/ca.key
```

### Step 3: Create a self-signed CA certificate (valid 10 years)

**NOTE:** Replace "subj" parameters with your organization information

```
openssl req -x509 -new -nodes -key ca/ca.key -sha256 -days 3650 \
  -out ca/ca.crt \
  -subj "/C=US/ST=CO/L=Boulder/O=NIST/OU=UAS/CN=UAS6-Internal-CA"
```

### Step 4: Create the Drone Server certificate (server-side TLS)

You must decide your server identity, but you can use both the IP address and DNS FQDN

- If you use https://192.168.40.20 then the server must include this address in SAN
- If you use a FQDN such as https://drone-server.local then this must be in SAN

```
cat > server/server.ext <<'EOF'
authorityKeyIdentifier=keyid,issuer
basicConstraints=CA:FALSE
keyUsage = digitalSignature, keyEncipherment
extendedKeyUsage = serverAuth
subjectAltName = @alt_names

[alt_names]
DNS.1 = drone-server.local
IP.1 = 192.168.40.20
EOF
```

### Step 5: Generate the server private key

```
openssl genrsa -out server/server.key 2048
chmod 600 server/server.key
```

### Step 6: Create the server Certificate Signing Request (CSR)

**NOTE:** Replace "subj" parameters with your organization information

```
openssl req -new -key server/server.key -out server/server.csr \
  -subj "/C=US/ST=CO/L=Boulder/O=NIST-PSCR/OU=UAS/CN=drone-server.local"
```

## Step 7: Sign the server CSR with your CA

```
openssl x509 -req -in server/server.csr -CA ca/ca.crt -CAkey ca/ca.key \
  -CAcreateserial -out server/server.crt -days 825 -sha256 \
  -extfile server/server.ext
```

## Configure Caddy for Mutual TLS

### Step 8: Install Caddy

``` bash
sudo apt update
sudo apt install -y caddy
```

### Step 9: Copy Certificates

``` bash
sudo mkdir -p /etc/caddy/certs
sudo cp ~/mtls/server/server.crt /etc/caddy/certs/
sudo cp ~/mtls/server/server.key /etc/caddy/certs/
sudo cp ~/mtls/ca/ca.crt /etc/caddy/certs/
sudo chown -R caddy:caddy /etc/caddy/certs
sudo chmod 600 /etc/caddy/certs/server.key
```

### Step 10: Edit or Copy Caddyfile

``` bash
sudo nano /etc/caddy/Caddyfile
```

Example configuration:

``` caddyfile
{
  # optional while debugging
  # debug
}

# Redirect HTTP to HTTPS for browser traffic
http://192.168.40.20 {
  redir https://192.168.40.20{uri} permanent
}

# Browser HTTPS (no client cert)
https://192.168.40.20 {
  tls /etc/caddy/mtls/server.crt /etc/caddy/mtls/server.key
  reverse_proxy 127.0.0.1:5000
}

# FoxNode ingest over mTLS (client cert required)
:8443 {
  tls /etc/caddy/mtls/server.crt /etc/caddy/mtls/server.key {
    client_auth {
      mode require_and_verify
      trust_pool file /etc/caddy/mtls/ca.crt
    }
  }

  reverse_proxy 127.0.0.1:5000
}
```

or copy the [Caddyfile](data_ferry/PKI_configuration/Caddyfile) to /etc/caddy/Caddyfile  

``` bash
sudo cp ~/data_ferry/PKI_configuration/Caddyfile /etc/caddy/Caddyfile
sudo chmod 600 /etc/caddy/Caddyfile
```

### Step 11: Enable and Restart Caddy

``` bash
sudo systemctl enable caddy
sudo systemctl restart caddy
sudo systemctl status caddy
```

## FoxNode Certificate Generation 

### Step 12: Copy and run the shell script to generate all of your FoxNode certificates

These commands will copy the generate_foxnode_client_certs.sh file to the ~mtls directory, change permissions, make executable, and remove CRLFs that may stop it from running.

```
cp ~/data_ferry/PKI_configuration/generate_foxnode_client_certs.sh ~/mtls/
cd ~/mtls
chmod +x generate_foxnode_client_certs.sh
sed -i 's/\r$//' generate_foxnode_client_certs.sh
./generate_foxnode_client_certs.sh
```

Follow on-screen prompts.

**NOTE:** Replace "subj" parameters with your organization information. You will need to edit the [FoxNode Cert Generator](/data_ferry/PKI_configuration/generate_foxnode_client_certs.sh) file. The cofiguration in the script can be found near **Line 146** near the bottom of the script.

### ALT Step 12: Manual Method, create FoxNode client certificates (one per FoxNode)

The below example uses FoxNode 19 as an example.

```
FN=foxnode19
openssl genrsa -out clients/${FN}.key 2048
chmod 600 clients/${FN}.key

openssl req -new -key clients/${FN}.key -out clients/${FN}.csr \
  -subj "/C=US/ST=CO/L=Boulder/O=NIST-PSCR/OU=FoxNode/CN=${FN}"
```

Then sign the client CSR with your CA

```
openssl x509 -req -in clients/${FN}.csr -CA ca/ca.crt -CAkey ca/ca.key \
  -CAcreateserial -out clients/${FN}.crt -days 825 -sha256 \
  -extfile clients/client.ext
```

### Step 13: Copy client certificates to FoxNodes

You will have to manually copy the client certificate for each FoxNode one at a time. To help expidite the transfer it is recommended to copy the client certificates from the Drone Server to a PC running the Arduino IDE. The copy process can be done using a secure transfer client such as WinSCP (Windows) or scp for Linux or MacOS systems.  

WinSCP Example:

Example scp command:
```
scp -r pscr@192.16840.20:/home/pscr/mstls/clients /local/directory/
```

Alternatively, you may use a USB thumb drive or similar removable media to mode the certificates; however, ensure to practice safe media transfer methods. It is recommended to delete thumb drive contents after the trasfer.  

## Next Steps

For configuring the client certs see the [FoxNode](/foxnode/README.md) documentation.

For further Data Ferry guides see the [next section](/data_ferry/server_management/README.md)



