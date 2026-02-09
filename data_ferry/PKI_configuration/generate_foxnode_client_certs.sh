#!/usr/bin/bash
# generate_client_certs.sh
#
# Generates and signs mTLS client certificates for FoxNode<N> using an internal CA.
# - Prompts for start/end FoxNode numbers (inclusive)
# - Creates clients/FoxNode<N>.key, .csr, .crt for each N
# - Uses ca/ca.crt + ca/ca.key and clients/client.ext
#
# Assumes you run this from your mtls working directory that contains:
#   ./ca/ca.crt
#   ./ca/ca.key
#   ./clients/client.ext
#
# Safe defaults:
# - 2048-bit RSA keys
# - Backdated NotBefore and long NotAfter to tolerate clock skew
#
# NOTE: FoxNode naming uses no zero padding: FoxNode1..FoxNode30
# NOTE: Replace subj information with your organizational information

set -euo pipefail

CA_CRT="ca/ca.crt"
CA_KEY="ca/ca.key"
CLIENTS_DIR="clients"
CLIENT_EXT="${CLIENTS_DIR}/client.ext"
DAYS=825
VALID_YEARS=10
BACKDATE_DAYS=365
KEY_BITS=2048

die() { echo "ERROR: $*" >&2; exit 1; }

require_file() {
  [[ -f "$1" ]] || die "Missing required file: $1"
}

prompt_int() {
  local varname="$1"
  local prompt="$2"
  local val
  while true; do
    read -r -p "$prompt" val
    [[ "$val" =~ ^[0-9]+$ ]] && { printf -v "$varname" "%s" "$val"; return; }
    echo "Please enter a non-negative integer."
  done
}

# --- Preconditions ---
require_file "$CA_CRT"
require_file "$CA_KEY"
require_file "$CLIENT_EXT"
mkdir -p "$CLIENTS_DIR"

OPENSSL_CA_DIR="ca"
mkdir -p "${OPENSSL_CA_DIR}/newcerts"
touch "${OPENSSL_CA_DIR}/index.txt"
[[ -f "${OPENSSL_CA_DIR}/serial" ]] || echo "1000" > "${OPENSSL_CA_DIR}/serial"

# Compute validity window in Zulu time. Requires GNU date (works on Raspberry Pi OS).
STARTDATE="$(date -u -d "${BACKDATE_DAYS} days ago" +%Y%m%d%H%M%SZ)"
ENDDATE="$(date -u -d "+${VALID_YEARS} years" +%Y%m%d%H%M%SZ)"

# Create a minimal openssl ca config on the fly so we can set start/end dates.
OPENSSL_CNF="$(mktemp)"
cleanup() { rm -f "$OPENSSL_CNF"; }
trap cleanup EXIT

cat > "$OPENSSL_CNF" <<'EOF'
[ ca ]
default_ca = CA_default

[ CA_default ]
dir               = ca
database          = $dir/index.txt
new_certs_dir     = $dir/newcerts
serial            = $dir/serial
private_key       = ca/ca.key
certificate       = ca/ca.crt
default_md        = sha256
policy            = policy_any
unique_subject    = no
copy_extensions   = copy
email_in_dn       = no

[ policy_any ]
countryName             = optional
stateOrProvinceName     = optional
localityName            = optional
organizationName        = optional
organizationalUnitName  = optional
commonName              = supplied
emailAddress            = optional
EOF

echo "FoxNode Client Certificate Generator (mTLS)"
echo "CA: $CA_CRT"
echo "CA key: $CA_KEY"
echo "Client ext: $CLIENT_EXT"
echo "NotBefore (backdated): $STARTDATE"
echo "NotAfter:              $ENDDATE"
echo

START=0
END=0
prompt_int START "Enter starting FoxNode number (e.g., 1): "
prompt_int END   "Enter ending FoxNode number (e.g., 30): "

(( END >= START )) || die "Ending number must be >= starting number."

echo
echo "About to generate client certs for: FoxNode${START} .. FoxNode${END}"
read -r -p "Continue? [y/N] " CONFIRM
[[ "${CONFIRM,,}" == "y" ]] || die "Aborted."

echo

for (( i=START; i<=END; i++ )); do
  FN="FoxNode${i}"
  KEY_PATH="${CLIENTS_DIR}/${FN}.key"
  CSR_PATH="${CLIENTS_DIR}/${FN}.csr"
  CRT_PATH="${CLIENTS_DIR}/${FN}.crt"

  echo "==> ${FN}"

  if [[ -f "$KEY_PATH" || -f "$CSR_PATH" || -f "$CRT_PATH" ]]; then
    echo "    Files already exist for ${FN}:"
    [[ -f "$KEY_PATH" ]] && echo "      - $KEY_PATH"
    [[ -f "$CSR_PATH" ]] && echo "      - $CSR_PATH"
    [[ -f "$CRT_PATH" ]] && echo "      - $CRT_PATH"
    read -r -p "    Overwrite existing files for ${FN}? [y/N] " OW
    if [[ "${OW,,}" != "y" ]]; then
      echo "    Skipping ${FN}"
      echo
      continue
    fi
    rm -f "$KEY_PATH" "$CSR_PATH" "$CRT_PATH"
  fi

  # 1) Generate private key
  openssl genrsa -out "$KEY_PATH" "$KEY_BITS"
  chmod 600 "$KEY_PATH"

  # 2) Create CSR
  openssl req -new -key "$KEY_PATH" -out "$CSR_PATH" \
    -subj "/C=US/ST=CO/L=Boulder/O=NIST/OU=FoxNode/CN=${FN}"

  # 3) Sign CSR with CA using openssl ca so we can set explicit NotBefore/NotAfter
  #    This greatly improves tolerance to server/client clock skew.
  openssl ca -batch -config "$OPENSSL_CNF" \
    -in "$CSR_PATH" -out "$CRT_PATH" \
    -startdate "$STARTDATE" -enddate "$ENDDATE" \
    -extfile "$CLIENT_EXT"

  echo "    Created:"
  echo "      - $CRT_PATH"
  echo "      - $KEY_PATH"
  echo
done

echo "Done."
echo "Client certs are in: ${CLIENTS_DIR}/"
