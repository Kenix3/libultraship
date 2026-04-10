import argparse
import base64
import hashlib
import json
import os
import shutil
import sys
import tempfile
import zipfile

from cryptography.hazmat.primitives import serialization
from cryptography.hazmat.primitives.asymmetric import ed25519

def update_zip_manifest(zip_path: str, public_key_path: str, checksum: str, signature: str):
    manifest_data = {}

    with zipfile.ZipFile(zip_path, 'r') as zin:
        if "manifest.json" in zin.namelist():
            try:
                existing_manifest = zin.read("manifest.json").decode('utf-8')
                manifest_data = json.loads(existing_manifest)
            except json.JSONDecodeError:
                print("Warning: Existing manifest.json is invalid JSON. Overwriting.")

    manifest_data["checksum"] = checksum
    manifest_data["signature"] = signature

    with open(public_key_path, "rb") as pk_file:
        public_key_data = pk_file.read()
        manifest_data["public_key"] = public_key_data.hex()

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
            return private_key
    except FileNotFoundError:
        print(f"Error: Private key file not found at '{key_path}'")
        sys.exit(1)
    except ValueError as e:
        print(f"Error loading key (incorrect password or invalid format): {e}")
        sys.exit(1)

def sign_data(private_key, data: bytes) -> str:
    signature = private_key.sign(data)
    return signature.hex()

def calculate_o2r_checksum(zip_path):
    try:
        hasher = hashlib.blake2b()
    except ValueError:
        print("Error: Unsupported hash algorithm")
        sys.exit(1)

    try:
        with zipfile.ZipFile(zip_path, 'r') as zf:
            all_entries = zf.namelist()
            filtered_files = [
                f for f in all_entries 
                if not f.endswith('/') and f != 'manifest.json'
            ]
            filtered_files.sort()

            for file_name in filtered_files:
                hasher.update(file_name.encode('utf-8'))
                with zf.open(file_name) as f:
                    for chunk in iter(lambda: f.read(8192), b""):
                        hasher.update(chunk)
        return hasher.hexdigest()
    except FileNotFoundError:
        print(f"Error: File '{zip_path}' not found.")
        sys.exit(1)
    except zipfile.BadZipFile:
        print(f"Error: '{zip_path}' is not a valid ZIP file or is corrupted.")
        sys.exit(1)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Generate a deterministic checksum, sign it, and inject it into the ZIP's manifest.json."
    )
    parser.add_argument("zip_file", help="Path to the target .zip file")
    parser.add_argument("private_key", help="Path to the PEM file containing the Ed25519 private key")
    parser.add_argument("public_key", help="Path to the PEM file containing the Ed25519 public key")
    parser.add_argument("-p", "--passphrase", help="Passphrase for the PEM file (if encrypted)", default=None)

    args = parser.parse_args()

    print(f"Processing {args.zip_file}...")

    checksum = calculate_o2r_checksum(args.zip_file)
    print(f"Calculated BLAKE2b: {checksum}")

    passphrase_bytes = args.passphrase.encode('utf-8') if args.passphrase else None
    private_key = load_private_key(args.private_key, password=passphrase_bytes)

    print("Signing checksum...")
    signature_str = sign_data(private_key, checksum.encode('utf-8'))

    print("Injecting signature and checksum into manifest.json...")
    update_zip_manifest(args.zip_file, args.public_key, checksum, signature_str)

    print("[SUCCESS] Zip file updated and signed successfully.")