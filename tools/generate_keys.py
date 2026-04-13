from cryptography.hazmat.primitives.asymmetric import ed25519
from cryptography.hazmat.primitives import serialization

def generate_keypair(private_out="private_key.pem", public_out="public_key.pem", password=None):
    print("Generating Ed25519 key pair...")

    private_key = ed25519.Ed25519PrivateKey.generate()

    if password:
        encryption = serialization.BestAvailableEncryption(password.encode('utf-8'))
    else:
        encryption = serialization.NoEncryption()

    with open(private_out, "wb") as f:
        f.write(private_key.private_bytes(
            encoding=serialization.Encoding.PEM,
            format=serialization.PrivateFormat.PKCS8,
            encryption_algorithm=encryption
        ))
    print(f"Saved private key to: {private_out}")

    public_key = private_key.public_key()
    with open(public_out, "wb") as f:
        f.write(public_key.public_bytes(
            encoding=serialization.Encoding.PEM,
            format=serialization.PublicFormat.SubjectPublicKeyInfo
        ))
    print(f"Saved public key to: {public_out}")

if __name__ == "__main__":
    generate_keypair(password=None)