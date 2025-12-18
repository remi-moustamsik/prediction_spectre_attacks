from Crypto.Cipher import PKCS1_OAEP
from Crypto.PublicKey import RSA


def run_rsa():
    print("=== RSA demo ===")
    message = b"Message de test RSA"

    key = RSA.generate(1024)  # clé privée
    public_key = key.publickey()

    cipher_enc = PKCS1_OAEP.new(public_key)
    ciphertext = cipher_enc.encrypt(message)

    cipher_dec = PKCS1_OAEP.new(key)
    plaintext = cipher_dec.decrypt(ciphertext)

    print(f"Plaintext  : {message}")
    print(f"Ciphertext (hex) : {ciphertext.hex()}")
    print(f"Decrypted  : {plaintext}")


if __name__ == "__main__":
    run_rsa()
