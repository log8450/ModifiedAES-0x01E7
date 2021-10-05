#include <iostream>
#include <fstream>

#define SIZE 16
#define ModifiedPx	0x01E7	//487

using namespace std;

const char* key_ = "key.bin";
const char* plain_ = "plain.bin";
const char* cipher_ = "cipher.bin";
const char* plain2_ = "plain2.bin";	// ����� ����������

unsigned char state[4][4];			// ���꿡 ����� state�� ����
unsigned char key[11][SIZE];		// Key Expansion���� �� ������ Key�� ����

const unsigned char MixColumnTable[4][4] = {
	{2,3,1,1},
	{1,2,3,1},
	{1,1,2,3},
	{3,1,1,2}
};
// MixColumn �ܰ迡�� ����� ��Ʈ����

const unsigned char MixColumnInverseTable[4][4] = {
	{14,11,13,9},
	{9,14,11,13},
	{13,9,14,11},
	{11,13,9,14}
};
// MixColumn ������ �ܰ迡�� ����� ��Ʈ����

const int matrix[8] = {
	0b11110001,
	0b11100011,
	0b11000111,
	0b10001111,
	0b00011111,
	0b00111110,
	0b01111100,
	0b11111000
};
// SubByte �ܰ迡�� ���� ������ ���� ��Ʈ����

const int matrixInverse[8] = {
	0b10100100,
	0b01001001,
	0b10010010,
	0b00100101,
	0b01001010,
	0b10010100,
	0b00101001,
	0b01010010
};
// SubByte ������ �ܰ迡�� ���� ��Ʈ����

int Division(int a, int b) {
	int p = 0;
	if (a < b) {
		int temp = a;
		a = b;
		b = temp;
	}
	for (int i = 8; i >= 0; i--) {
		if (a > (a ^ (b << i))) {
			a = (a ^ (b << i));
			p += 1 << i;
		}
	}
	return p;
}
// a�� b�� ���� ���� �������ִ� �Լ�

int deg(int bp) {
	for (int i = 16; i >= 0; i--) {
		if ((bp & (1 << i)) != 0) return i;
	}
	return 0;
}
// �������� ��Ÿ�� ���� ���� ���� ������ �������ִ� �Լ�

bool carry(int a) {
	if (a & 0x100) return 1;
	else return 0;
}
// �ʵ� ���� ��꿡�� ĳ���� �߻��ϸ� 1��,�� �߻����� ������ 0�� �����մϴ�


unsigned char InverseElement(int number) {	// �ش��ϴ� �ʵ忡�� ���ڷ� �Է¹��� ���� ������ �������ִ� �Լ�
	if (number == 0) return 0;
	int u = ModifiedPx;
	int	v = number;
	int	g1 = 1;
	int	g2 = 0;
	int	h1 = 0;
	int	h2 = 1;
	while (u != 0) {
		int j = deg(u) - deg(v);
		if (j < 0) {		// ���ڰ� �ʵ��� �������� ������ ������
			int temp = u;
			u = v;
			v = temp;
			temp = g1;
			g1 = g2;
			g2 = temp;
			temp = h1;
			h1 = h2;
			h2 = temp;
			j = -j;
		}					// Ȯ�� ��Ŭ���带 ������ݴϴ�
		u = u ^ (v << j);
		g1 = g1 ^ (g2 << j);
		h1 = h1 ^ (h2 << j);
	}
	int d = v;
	int g = g2;
	int h = h2;

	return h;				// �������� ������ �������ݴϴ�
}

unsigned char Multiplication(unsigned char a, unsigned char b) {	// a�� b�� �ʵ���� ���� �������ݴϴ�
	int buf = ModifiedPx & 0xff;
	int f[8] = { 0,0,0,0,0,0,0,0 };

	f[0] = a;
	for (int i = 1; i < 8; i++) {
		f[i] = f[i - 1] << 1;
		if (carry(f[i])) {
			f[i] &= 0xff;
			f[i] ^= buf;
		}
	}		// f��� �迭�� ���� �ڸ����� ���� �´� ���� MOD���� �� �־��ݴϴ�

	int res = 0;
	for (int i = 0; i < 8; i++) {
		int mask = 1 << i;
		if ((b & mask) != 0) {
			res ^= f[i];
		}	// ����ũ�� ����Ͽ� b�� �ڸ����� 1�̶�� �� �ڸ����� �´� ���� �����ݴϴ� (GF(2^8)�̹Ƿ� XOR�� ������)
	}
	return res;
}

int Mod(int Px_, int number) {				// Key Expansion�� ����� Mod�� �����ִ� �Լ��Դϴ�.
	if (number < 0x100) return number;		// ���� 8������ �۴ٸ� �״�� �������ݴϴ�
	int q = Division(Px_, number);
	int remain = number ^ Multiplication(q, Px_);
	return remain;
}	// number��� ���� (P(x)�� ���� ��) + (������)
	// �������� ���ϱ� ���� number���� (P(x)�� ���� ��)�� ���ݴϴ� (GF(2^8)�̹Ƿ� XOR�� ������)

int countOne(int a) {						// SubByte �ܰ迡�� ������ ��Ʈ������ ������ ��, 1�� ������ ���� �������ݴϴ�
	int count = 0;
	for (int i = 8; i >= 0; i--) {
		if (a & (1 << i)) {		// �� ��Ʈ�� ���鼭
			count++;			// ī��Ʈ�� ������ŵ�ϴ�
		}
	}
	return count;
}

int MultiMatrix(int a) {					// SubByte �ܰ迡�� ���� ������ ��Ʈ������ �����ݴϴ�.
	int number = 0;
	for (int i = 0; i < 8; i++) {
		number += (countOne(a & matrix[i]) % 2) << i;
	}
	number ^= 0b01100011;					// 0x63�� XOR ���ݴϴ�
	return number;
}

int MultiMatrixInverse(int a) {				// SubByte �ܰ迡�� �Է����� ���� ���� ��Ʈ������ �����ݴϴ�.
	int number = 0;
	for (int i = 0; i < 8; i++) {
		number += (countOne(a & matrixInverse[i]) % 2) << i;
	}
	number ^= 0b00000101;					// 0x05�� XOR ���ݴϴ�
	return number;
}

int SBOX(unsigned char input) {							// SBOX �Լ��� �Է����� ���� ������ ������ ��Ʈ������ ���Ͽ� �������ݴϴ�
	return MultiMatrix(InverseElement(input));			// SBOX ������ ����� ���� �ƴ� �¶������� �����ϰ� �˴ϴ�
}

int SBOXInverse(unsigned char input) {					// SBOXInverse �Լ��� �Է����� ���� ���ڿ� ��Ʈ������ ���� ���� ������ ã���ϴ�
	return InverseElement(MultiMatrixInverse(input));	// SBOX ������ ����� ���� �ƴ� �¶������� �����ϰ� �˴ϴ�
}

void SubBytes() {										// SubBytes �ܰ� : �� state�� ���� S-box�� ����ϴ� ��ó�� ���� �ٲ��ݴϴ�
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			state[i][j] = SBOX(state[i][j]);
		}
	}
}

void SubBytesInverse() {								// SubBytes ������ �ܰ� : �� state�� ���� S-box�� ����ϴ� ��ó�� ���� �ٲ��ݴϴ�
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			state[i][j] = SBOXInverse(state[i][j]);
		}
	}
}

void ShiftRows() {										// ShiftRows �ܰ� : �� state�� ���� Shift Row�� ���꿡 �°� �̵������ݴϴ�
	unsigned char temp;

	temp = state[1][0];
	state[1][0] = state[1][1];
	state[1][1] = state[1][2];
	state[1][2] = state[1][3];
	state[1][3] = temp;

	temp = state[2][0];
	state[2][0] = state[2][2];
	state[2][2] = temp;
	temp = state[2][1];
	state[2][1] = state[2][3];
	state[2][3] = temp;

	temp = state[3][0];
	state[3][0] = state[3][3];
	state[3][3] = state[3][2];
	state[3][2] = state[3][1];
	state[3][1] = temp;
}

void ShiftRowsInverse() {								// ShiftRows ������ �ܰ� : �� state�� ���� Shift Row�� �����꿡 �°� �̵������ݴϴ�
	unsigned char temp;

	temp = state[1][3];
	state[1][3] = state[1][2];
	state[1][2] = state[1][1];
	state[1][1] = state[1][0];
	state[1][0] = temp;

	temp = state[2][0];
	state[2][0] = state[2][2];
	state[2][2] = temp;
	temp = state[2][1];
	state[2][1] = state[2][3];
	state[2][3] = temp;

	temp = state[3][0];
	state[3][0] = state[3][1];
	state[3][1] = state[3][2];
	state[3][2] = state[3][3];
	state[3][3] = temp;
}

void MixColumns() {										// MixColumns �ܰ� : state�� ����� �����ݴϴ�.
	unsigned char tempMatrix[4][4];
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			tempMatrix[i][j] = 0;
		}
	}		// �ӽ� ��Ʈ������ �����ϰ�
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			for (int k = 0; k < 4; k++) {
				int temp = Multiplication(MixColumnTable[i][k], state[k][j]);
				tempMatrix[i][j] ^= temp;
			}
		}	// MixColumn�� �ش��ϴ� ����� �����־� �ӽ� ��Ʈ������ �����մϴ�
	}
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			state[i][j] = tempMatrix[i][j];
		}	// �ӽ� ��Ʈ������ ���� state�� �����ɴϴ�
	}
}

void MixColumnsInverse() {								// MixColumns ������ �ܰ� : state�� ����� �����ݴϴ�.
	unsigned char tempMatrix[4][4];
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			tempMatrix[i][j] = 0;
		}
	}		// �ӽ� ��Ʈ������ �����ϰ�
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			for (int k = 0; k < 4; k++) {
				int temp = Multiplication(MixColumnInverseTable[i][k], state[k][j]);
				tempMatrix[i][j] ^= temp;
			}
		}	// MixColumn �����꿡 �ش��ϴ� ����� �����־� �ӽ� ��Ʈ������ �����մϴ�
	}
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			state[i][j] = tempMatrix[i][j];
		}	// �ӽ� ��Ʈ������ ���� state�� �����ɴϴ�
	}
}

void AddRoundKey(int round) {							// AddRoundKey �ܰ�
	int i, j;
	for (i = 0; i < 4; i++) {
		for (j = 0; j < 4; j++) {						// Ű�� �̸� ����� �� ���Դϴ�
			state[j][i] ^= key[round][i * 4 + j];		// �ش��ϴ� Ű�� XOR���ݴϴ�
		}
	}
}

/////////////////////////////////////////// ���� �Լ� ///////////////////////////////////////////

int main(int argc, char* argv[]) {
	char control;			// ���

	cout << "Welcome to AES(Modified) Program!" << endl << endl;

	cout << "Please enter the function you want to use" << endl
		<< "+ Encode System : e" << endl
		<< "| Decode System : d" << endl
		<< "| Help : h " << endl
		<< "+ Exit : any key" << endl << endl
		<< "function : ";

	cin >> control;
	cin.ignore();
	cout << endl;

	FILE* keyFile;
	FILE* cipherFile;

	unsigned char plain[SIZE];
	unsigned char cipher[SIZE];

	if (control == 'e') {	// option e
		FILE* plainFile;		// ��ȣȭ ��忡���� �� ������ �о� �ɴϴ�

		if (fopen_s(&cipherFile, cipher_, "w+b") != 0) {
			cout << "File Open Error" << endl;
			return 1;
		}	// ������ ��ȣ�� ������ ���� ���� �����ݴϴ�

		if (fopen_s(&keyFile, key_, "rb") != 0) {
			cout << "File Open Error" << endl;
			return 1;
		}

		fread(key, sizeof(char), SIZE, keyFile);

		if (fopen_s(&plainFile, plain_, "rb") != 0) {
			cout << "File Open Error" << endl;
			return 1;
		}

		int readChar = fread(plain, sizeof(char), SIZE, plainFile);
		// Ű ���ϰ� �� ������ �迭�� �о�ɴϴ�

		// Key Expansion �ܰ�
		unsigned char temp[4];
		unsigned char RC[11];

		RC[1] = 1;	// ó�� RC1�� ���� 0b00000001

		for (int i = 2; i < 11; i++) {
			RC[i] = Mod(ModifiedPx, (RC[i - 1] << 1));
		}	// �� ���� ���� �� �� RC�� ���� �������� 1�� ����Ʈ�� ���� ����ϴµ�, �̶� 8��° ��Ʈ�� �Ѿ�ٸ� �ʵ忡 �°� Mod�������ݴϴ�

		for (int r = 0; r < 10; r++) {
			temp[0] = key[r][13];
			temp[1] = key[r][14];
			temp[2] = key[r][15];
			temp[3] = key[r][12];		// ��ġ�̵�
			temp[0] = SBOX(temp[0]);
			temp[1] = SBOX(temp[1]);
			temp[2] = SBOX(temp[2]);
			temp[3] = SBOX(temp[3]);	// S-box ���
			temp[0] ^= RC[r + 1];		// XOR
			key[r + 1][0] = temp[0] ^ key[r][0];
			key[r + 1][1] = temp[1] ^ key[r][1];
			key[r + 1][2] = temp[2] ^ key[r][2];
			key[r + 1][3] = temp[3] ^ key[r][3];
			for (int i = 4; i < 16; i++) {
				key[r + 1][i] = key[r + 1][i - 4] ^ key[r][i];
			}		// �� ���忡 �°� Ű�� �������ݴϴ�
		}

		while (readChar != 0) {
			// ��ȣȭ ���� ����
			if (readChar < 16) {
				for (int i = readChar; i < SIZE; i++) {
					plain[i] = 0;
				}
			}

			for (int i = 0; i < 4; i++) {
				for (int j = 0; j < 4; j++) {
					state[j][i] = plain[i * 4 + j];
				}
			}	// state�� �˸��� ��ġ�� ���� ���� �־��ݴϴ�
			
			AddRoundKey(0);

			for (int r = 1; r < 10; r++) {	// ���� ����
				SubBytes();
				ShiftRows();
				MixColumns();
				AddRoundKey(r);
			}	// SubBytes, ShiftRows, MixColumns, AddRoundKey�� 9������� �����մϴ�.

			SubBytes();
			ShiftRows();
			AddRoundKey(10);

			// ������ 10����� MixColumns�� ���� �ʽ��ϴ�

			for (int i = 0; i < 4; i++) {
				for (int j = 0; j < 4; j++) {
					cipher[i * 4 + j] = state[j][i];
				}
			}	// �� state�� ���� ������ �°� cipher �迭�� �־��ݴϴ�

			fwrite(cipher, sizeof(char), SIZE, cipherFile);
			// ��ȣ���� ���Ͽ� �����ݴϴ�

			readChar = fread(plain, sizeof(char), SIZE, plainFile); // ���� ����
		}

		cout << "Encryption using AES method is completed." << endl
			<< "The output file is cipher.bin" << endl;

		return 0;	// ���α׷� ����
	}

	/////////////////////////////////////////////////////////////////////////////////////////

	else if (control == 'd') {	// option d
		FILE* plain2File;			// ��ȣȭ ��忡���� ���� ������ ���� ��ũ���͸� �����մϴ�

		if (fopen_s(&plain2File, plain2_, "w+b") != 0) {
			cout << "File Open Error" << endl;
			return 1;
		}	// ������ �� ������ ���� ���� �����ݴϴ�

		if (fopen_s(&keyFile, key_, "rb") != 0) {
			cout << "File Open Error" << endl;
			return 1;
		}

		fread(key, sizeof(char), SIZE, keyFile);

		if (fopen_s(&cipherFile, cipher_, "rb") != 0) {
			cout << "File Open Error" << endl;
			return 1;
		}

		int readChar = fread(cipher, sizeof(char), SIZE, cipherFile);
		// Ű ���ϰ� �� ������ �迭�� �о�ɴϴ�

		// Key Expansion �ܰ�
		unsigned char temp[4];
		unsigned char RC[11];

		RC[1] = 1;	// ó�� RC1�� ���� 0b00000001

		for (int i = 2; i < 11; i++) {
			RC[i] = Mod(ModifiedPx, (RC[i - 1] << 1));
		}	// �� ���� ���� �� �� RC�� ���� �������� 1�� ����Ʈ�� ���� ����ϴµ�, �̶� 8��° ��Ʈ�� �Ѿ�ٸ� �ʵ忡 �°� Mod�������ݴϴ�

		for (int r = 0; r < 10; r++) {
			temp[0] = key[r][13];
			temp[1] = key[r][14];
			temp[2] = key[r][15];
			temp[3] = key[r][12];		// ��ġ�̵�
			temp[0] = SBOX(temp[0]);
			temp[1] = SBOX(temp[1]);
			temp[2] = SBOX(temp[2]);
			temp[3] = SBOX(temp[3]);	// S-box ���
			temp[0] ^= RC[r + 1];		// XOR
			key[r + 1][0] = temp[0] ^ key[r][0];
			key[r + 1][1] = temp[1] ^ key[r][1];
			key[r + 1][2] = temp[2] ^ key[r][2];
			key[r + 1][3] = temp[3] ^ key[r][3];
			for (int i = 4; i < 16; i++) {
				key[r + 1][i] = key[r + 1][i - 4] ^ key[r][i];
			}		// �� ���忡 �°� Ű�� �������ݴϴ�
		}

		while (readChar != 0) {
			// ��ȣȭ ���� ����
			if (readChar < 16) {
				for (int i = readChar; i < SIZE; i++) {
					plain[i] = 0;
				}
			}

			for (int i = 0; i < 4; i++) {
				for (int j = 0; j < 4; j++) {
					state[j][i] = cipher[i * 4 + j];
				}
			}	// state�� �˸��� ��ġ�� ��ȣ���� ���� �־��ݴϴ�

			AddRoundKey(10);	// �Ųٷ� 10���� ����Ű�� �������ݴϴ�

			for (int r = 1; r < 10; r++) {	// ���� ����
				ShiftRowsInverse();
				SubBytesInverse();
				AddRoundKey(10 - r);
				MixColumnsInverse();
			}

			ShiftRowsInverse();
			SubBytesInverse();
			AddRoundKey(0);
			// ������ 10����� MixColumns�� ���� �ʽ��ϴ�

			for (int i = 0; i < 4; i++) {
				for (int j = 0; j < 4; j++) {
					plain[i * 4 + j] = state[j][i];
				}
			}	// �� state�� ���� ������ �°� plain �迭�� �־��ݴϴ�

			fwrite(plain, sizeof(char), SIZE, plain2File);
			// ��ȣ���� ���Ͽ� �����ݴϴ�

			readChar = fread(cipher, sizeof(char), SIZE, cipherFile); // ���� ����
		}

		cout << "Decryption using AES method is completed." << endl
			<< "The output file is plain2.bin" << endl;

		return 0;	// ���α׷� ����
	}

	else if (control == 'h') {
	cout << "This is a modified AES system." << endl
		<< "The equation for AES used by default is x^8+x^4+x^3+x+1" << endl
		<< "However, the equation used for implementation in this program is x^8+x^7+x^6+x^5+x^2+x+1" << endl
		<< "Using plain.bin and key.bin to generate cipher.bin is encoding." << endl
		<< "Using cipher.bin and key.bin to generate plain2.bin is decoding. " << endl;
	}
}