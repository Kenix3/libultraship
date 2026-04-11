import argparse
import hashlib
import json
import os
import shutil
import sys
import tempfile
import zipfile

from cryptography.hazmat.primitives import serialization
from cryptography.hazmat.primitives.asymmetric import ed25519

def update_zip_manifest(zip_path: str, public_key_hex: str, checksum_hex: str, signature_hex: str):
    manifest_data = {}

    with zipfile.ZipFile(zip_path, 'r') as zin:
        if "manifest.json" in zin.namelist():
            try:
                existing_manifest = zin.read("manifest.json").decode('utf-8')
                manifest_data = json.loads(existing_manifest)
            except json.JSONDecodeError:
                print("Warning: Existing manifest.json is invalid JSON. Overwriting.")

    manifest_data["checksum"] = checksum_hex
    manifest_data["signature"] = signature_hex
    manifest_data["public_key"] = public_key_hex

    fd, temp_path = tempfile.mkstemp(suffix=".zip")
    os.close(fd)

    try:
        with zipfile.ZipFile(zip_path, 'r') as zin:
            with zipfile.ZipFile(temp_path, 'w', zipfile.ZIP_DEFLATED) as zout:
                for item in zin.infolist():
                    if item.filename != "manifest.json":
                        zout.writestr(item, zin.read(item.filename))
                new_manifest_bytes = json.dumps(manifest_data, indent=4).encode('utf-8')
                zout.writestr("manifest.json", new_manifest_bytes)
        shutil.move(temp_path, zip_path)
    except Exception as e:
        if os.path.exists(temp_path):
            os.remove(temp_path)
        raise e

def load_private_key(key_path: str, password: bytes = None):
    try:
        with open(key_path, "rb") as key_file:
            private_key = serialization.load_pem_private_key(
                key_file.read(),
                password=password
            )
            if not isinstance(private_key, ed25519.Ed25519PrivateKey):
                print(f"Error: The key at '{key_path}' is not an Ed25519 private key.")
                sys.exit(1)
            return private_key
    except FileNotFoundError:
        print(f"Error: Private key file not found at '{key_path}'")
        sys.exit(1)

def calculate_o2r_checksum(zip_path) -> bytes:
    hasher = hashlib.blake2b()
    with zipfile.ZipFile(zip_path, 'r') as zf:
        all_entries = zf.namelist()
        filtered_files = [f for f in all_entries if not f.endswith('/') and f != 'manifest.json']
        filtered_files.sort()

        for file_name in filtered_files:
            hasher.update(file_name.encode('utf-8'))
            with zf.open(file_name) as f:
                for chunk in iter(lambda: f.read(8192), b""):
                    hasher.update(chunk)

    return hasher.digest()

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Sign a ZIP and inject into manifest.json.")
    parser.add_argument("zip_file", help="Path to the target .zip file")
    parser.add_argument("private_key", help="Path to the PEM private key")
    parser.add_argument("-p", "--passphrase", help="Passphrase for the PEM file", default=None)

    args = parser.parse_args()
    print(f"Processing {args.zip_file}...")

    raw_checksum = calculate_o2r_checksum(args.zip_file)
    hex_checksum = raw_checksum.hex()

    passphrase_bytes = args.passphrase.encode('utf-8') if args.passphrase else None
    private_key = load_private_key(args.private_key, password=passphrase_bytes)

    public_key_obj = private_key.public_key()
    raw_public_key = public_key_obj.public_bytes(
        encoding=serialization.Encoding.Raw,
        format=serialization.PublicFormat.Raw
    )
    public_key_hex = raw_public_key.hex()
    signature_str = private_key.sign(raw_checksum).hex()
    update_zip_manifest(args.zip_file, public_key_hex, hex_checksum, signature_str)
    print("[SUCCESS] Zip signed successfully.")