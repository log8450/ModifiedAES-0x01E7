#include <iostream>
#include <fstream>

#define SIZE 16
#define ModifiedPx	0x01E7	//487

using namespace std;

const char* key_ = "key.bin";
const char* plain_ = "plain.bin";
const char* cipher_ = "cipher.bin";
const char* plain2_ = "plain2.bin";	// 입출력 데이터파일

unsigned char state[4][4];			// 연산에 사용할 state를 저장
unsigned char key[11][SIZE];		// Key Expansion으로 각 라운드의 Key를 저장

const unsigned char MixColumnTable[4][4] = {
	{2,3,1,1},
	{1,2,3,1},
	{1,1,2,3},
	{3,1,1,2}
};
// MixColumn 단계에서 사용할 매트릭스

const unsigned char MixColumnInverseTable[4][4] = {
	{14,11,13,9},
	{9,14,11,13},
	{13,9,14,11},
	{11,13,9,14}
};
// MixColumn 역연산 단계에서 사용할 매트릭스

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
// SubByte 단계에서 구한 역원에 곱할 매트릭스

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
// SubByte 역연산 단계에서 곱할 매트릭스

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
// a를 b로 나눈 몫을 리턴해주는 함수

int deg(int bp) {
	for (int i = 16; i >= 0; i--) {
		if ((bp & (1 << i)) != 0) return i;
	}
	return 0;
}
// 이진수로 나타낸 값의 가장 높은 차수를 리턴해주는 함수

bool carry(int a) {
	if (a & 0x100) return 1;
	else return 0;
}
// 필드 곱셈 계산에서 캐리가 발생하면 1을,ㅣ 발생하지 않으면 0을 리턴합니다


unsigned char InverseElement(int number) {	// 해당하는 필드에서 인자로 입력받은 수의 역원을 리턴해주는 함수
	if (number == 0) return 0;
	int u = ModifiedPx;
	int	v = number;
	int	g1 = 1;
	int	g2 = 0;
	int	h1 = 0;
	int	h2 = 1;
	while (u != 0) {
		int j = deg(u) - deg(v);
		if (j < 0) {		// 숫자가 필드의 차수보다 낮아질 때까지
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
		}					// 확장 유클리드를 사용해줍니다
		u = u ^ (v << j);
		g1 = g1 ^ (g2 << j);
		h1 = h1 ^ (h2 << j);
	}
	int d = v;
	int g = g2;
	int h = h2;

	return h;				// 마지막의 역원을 리턴해줍니다
}

unsigned char Multiplication(unsigned char a, unsigned char b) {	// a와 b의 필드상의 곱을 리턴해줍니다
	int buf = ModifiedPx & 0xff;
	int f[8] = { 0,0,0,0,0,0,0,0 };

	f[0] = a;
	for (int i = 1; i < 8; i++) {
		f[i] = f[i - 1] << 1;
		if (carry(f[i])) {
			f[i] &= 0xff;
			f[i] ^= buf;
		}
	}		// f라는 배열에 각각 자리수의 값에 맞는 값을 MOD연산 후 넣어줍니다

	int res = 0;
	for (int i = 0; i < 8; i++) {
		int mask = 1 << i;
		if ((b & mask) != 0) {
			res ^= f[i];
		}	// 마스크를 사용하여 b의 자리수가 1이라면 그 자리수에 맞는 값을 더해줍니다 (GF(2^8)이므로 XOR과 동일함)
	}
	return res;
}

int Mod(int Px_, int number) {				// Key Expansion에 사용할 Mod를 구해주는 함수입니다.
	if (number < 0x100) return number;		// 값이 8차보다 작다면 그대로 리턴해줍니다
	int q = Division(Px_, number);
	int remain = number ^ Multiplication(q, Px_);
	return remain;
}	// number라는 수는 (P(x)와 몫의 곱) + (나머지)
	// 나머지를 구하기 위해 number에서 (P(x)와 몫의 곱)을 빼줍니다 (GF(2^8)이므로 XOR과 동일함)

int countOne(int a) {						// SubByte 단계에서 역원에 매트릭스를 곱해줄 때, 1의 개수를 세어 리턴해줍니다
	int count = 0;
	for (int i = 8; i >= 0; i--) {
		if (a & (1 << i)) {		// 한 비트씩 보면서
			count++;			// 카운트를 증가시킵니다
		}
	}
	return count;
}

int MultiMatrix(int a) {					// SubByte 단계에서 구한 역원에 매트릭스를 곱해줍니다.
	int number = 0;
	for (int i = 0; i < 8; i++) {
		number += (countOne(a & matrix[i]) % 2) << i;
	}
	number ^= 0b01100011;					// 0x63도 XOR 해줍니다
	return number;
}

int MultiMatrixInverse(int a) {				// SubByte 단계에서 입력으로 받은 수에 매트릭스를 곱해줍니다.
	int number = 0;
	for (int i = 0; i < 8; i++) {
		number += (countOne(a & matrixInverse[i]) % 2) << i;
	}
	number ^= 0b00000101;					// 0x05도 XOR 해줍니다
	return number;
}

int SBOX(unsigned char input) {							// SBOX 함수는 입력으로 받은 숫자의 역원에 매트릭스를 곱하여 리턴해줍니다
	return MultiMatrix(InverseElement(input));			// SBOX 구조를 사용한 것이 아닌 온라인으로 연산하게 됩니다
}

int SBOXInverse(unsigned char input) {					// SBOXInverse 함수는 입력으로 받은 숫자에 매트릭스를 곱한 값의 역원을 찾습니다
	return InverseElement(MultiMatrixInverse(input));	// SBOX 구조를 사용한 것이 아닌 온라인으로 연산하게 됩니다
}

void SubBytes() {										// SubBytes 단계 : 각 state의 값을 S-box를 사용하는 것처럼 값을 바꿔줍니다
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			state[i][j] = SBOX(state[i][j]);
		}
	}
}

void SubBytesInverse() {								// SubBytes 역연산 단계 : 각 state의 값을 S-box를 사용하는 것처럼 값을 바꿔줍니다
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			state[i][j] = SBOXInverse(state[i][j]);
		}
	}
}

void ShiftRows() {										// ShiftRows 단계 : 각 state의 값을 Shift Row의 연산에 맞게 이동시켜줍니다
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

void ShiftRowsInverse() {								// ShiftRows 역연산 단계 : 각 state의 값을 Shift Row의 역연산에 맞게 이동시켜줍니다
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

void MixColumns() {										// MixColumns 단계 : state에 행렬을 곱해줍니다.
	unsigned char tempMatrix[4][4];
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			tempMatrix[i][j] = 0;
		}
	}		// 임시 매트릭스를 생성하고
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			for (int k = 0; k < 4; k++) {
				int temp = Multiplication(MixColumnTable[i][k], state[k][j]);
				tempMatrix[i][j] ^= temp;
			}
		}	// MixColumn에 해당하는 행렬을 곱해주어 임시 매트릭스에 저장합니다
	}
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			state[i][j] = tempMatrix[i][j];
		}	// 임시 매트릭스의 값을 state로 가져옵니다
	}
}

void MixColumnsInverse() {								// MixColumns 역연산 단계 : state에 행렬을 곱해줍니다.
	unsigned char tempMatrix[4][4];
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			tempMatrix[i][j] = 0;
		}
	}		// 임시 매트릭스를 생성하고
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			for (int k = 0; k < 4; k++) {
				int temp = Multiplication(MixColumnInverseTable[i][k], state[k][j]);
				tempMatrix[i][j] ^= temp;
			}
		}	// MixColumn 역연산에 해당하는 행렬을 곱해주어 임시 매트릭스에 저장합니다
	}
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			state[i][j] = tempMatrix[i][j];
		}	// 임시 매트릭스의 값을 state로 가져옵니다
	}
}

void AddRoundKey(int round) {							// AddRoundKey 단계
	int i, j;
	for (i = 0; i < 4; i++) {
		for (j = 0; j < 4; j++) {						// 키는 미리 만들어 둘 것입니다
			state[j][i] ^= key[round][i * 4 + j];		// 해당하는 키와 XOR해줍니다
		}
	}
}

/////////////////////////////////////////// 메인 함수 ///////////////////////////////////////////

int main(int argc, char* argv[]) {
	char control;			// 기능

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
		FILE* plainFile;		// 암호화 모드에서는 평문 파일을 읽어 옵니다

		if (fopen_s(&cipherFile, cipher_, "w+b") != 0) {
			cout << "File Open Error" << endl;
			return 1;
		}	// 저장할 암호문 파일을 쓰기 모드로 열어줍니다

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
		// 키 파일과 평문 파일을 배열에 읽어옵니다

		// Key Expansion 단계
		unsigned char temp[4];
		unsigned char RC[11];

		RC[1] = 1;	// 처음 RC1의 값은 0b00000001

		for (int i = 2; i < 11; i++) {
			RC[i] = Mod(ModifiedPx, (RC[i - 1] << 1));
		}	// 그 다음 값은 그 전 RC의 값을 왼쪽으로 1번 시프트한 값을 사용하는데, 이때 8번째 비트를 넘어간다면 필드에 맞게 Mod연산해줍니다

		for (int r = 0; r < 10; r++) {
			temp[0] = key[r][13];
			temp[1] = key[r][14];
			temp[2] = key[r][15];
			temp[3] = key[r][12];		// 위치이동
			temp[0] = SBOX(temp[0]);
			temp[1] = SBOX(temp[1]);
			temp[2] = SBOX(temp[2]);
			temp[3] = SBOX(temp[3]);	// S-box 통과
			temp[0] ^= RC[r + 1];		// XOR
			key[r + 1][0] = temp[0] ^ key[r][0];
			key[r + 1][1] = temp[1] ^ key[r][1];
			key[r + 1][2] = temp[2] ^ key[r][2];
			key[r + 1][3] = temp[3] ^ key[r][3];
			for (int i = 4; i < 16; i++) {
				key[r + 1][i] = key[r + 1][i - 4] ^ key[r][i];
			}		// 각 라운드에 맞게 키를 생성해줍니다
		}

		while (readChar != 0) {
			// 암호화 시작 상태
			if (readChar < 16) {
				for (int i = readChar; i < SIZE; i++) {
					plain[i] = 0;
				}
			}

			for (int i = 0; i < 4; i++) {
				for (int j = 0; j < 4; j++) {
					state[j][i] = plain[i * 4 + j];
				}
			}	// state의 알맞은 위치에 평문의 값을 넣어줍니다
			
			AddRoundKey(0);

			for (int r = 1; r < 10; r++) {	// 라운드 시작
				SubBytes();
				ShiftRows();
				MixColumns();
				AddRoundKey(r);
			}	// SubBytes, ShiftRows, MixColumns, AddRoundKey를 9라운드까지 진행합니다.

			SubBytes();
			ShiftRows();
			AddRoundKey(10);

			// 마지막 10라운드는 MixColumns를 하지 않습니다

			for (int i = 0; i < 4; i++) {
				for (int j = 0; j < 4; j++) {
					cipher[i * 4 + j] = state[j][i];
				}
			}	// 각 state의 값을 순서에 맞게 cipher 배열에 넣어줍니다

			fwrite(cipher, sizeof(char), SIZE, cipherFile);
			// 암호문을 파일에 적어줍니다

			readChar = fread(plain, sizeof(char), SIZE, plainFile); // 다음 루프
		}

		cout << "Encryption using AES method is completed." << endl
			<< "The output file is cipher.bin" << endl;

		return 0;	// 프로그램 종료
	}

	/////////////////////////////////////////////////////////////////////////////////////////

	else if (control == 'd') {	// option d
		FILE* plain2File;			// 복호화 모드에서는 평문을 저장할 파일 디스크립터를 선언합니다

		if (fopen_s(&plain2File, plain2_, "w+b") != 0) {
			cout << "File Open Error" << endl;
			return 1;
		}	// 저장할 평문 파일을 쓰기 모드로 열어줍니다

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
		// 키 파일과 평문 파일을 배열에 읽어옵니다

		// Key Expansion 단계
		unsigned char temp[4];
		unsigned char RC[11];

		RC[1] = 1;	// 처음 RC1의 값은 0b00000001

		for (int i = 2; i < 11; i++) {
			RC[i] = Mod(ModifiedPx, (RC[i - 1] << 1));
		}	// 그 다음 값은 그 전 RC의 값을 왼쪽으로 1번 시프트한 값을 사용하는데, 이때 8번째 비트를 넘어간다면 필드에 맞게 Mod연산해줍니다

		for (int r = 0; r < 10; r++) {
			temp[0] = key[r][13];
			temp[1] = key[r][14];
			temp[2] = key[r][15];
			temp[3] = key[r][12];		// 위치이동
			temp[0] = SBOX(temp[0]);
			temp[1] = SBOX(temp[1]);
			temp[2] = SBOX(temp[2]);
			temp[3] = SBOX(temp[3]);	// S-box 통과
			temp[0] ^= RC[r + 1];		// XOR
			key[r + 1][0] = temp[0] ^ key[r][0];
			key[r + 1][1] = temp[1] ^ key[r][1];
			key[r + 1][2] = temp[2] ^ key[r][2];
			key[r + 1][3] = temp[3] ^ key[r][3];
			for (int i = 4; i < 16; i++) {
				key[r + 1][i] = key[r + 1][i - 4] ^ key[r][i];
			}		// 각 라운드에 맞게 키를 생성해줍니다
		}

		while (readChar != 0) {
			// 암호화 시작 상태
			if (readChar < 16) {
				for (int i = readChar; i < SIZE; i++) {
					plain[i] = 0;
				}
			}

			for (int i = 0; i < 4; i++) {
				for (int j = 0; j < 4; j++) {
					state[j][i] = cipher[i * 4 + j];
				}
			}	// state의 알맞은 위치에 암호문의 값을 넣어줍니다

			AddRoundKey(10);	// 거꾸로 10부터 라운드키를 적용해줍니다

			for (int r = 1; r < 10; r++) {	// 라운드 시작
				ShiftRowsInverse();
				SubBytesInverse();
				AddRoundKey(10 - r);
				MixColumnsInverse();
			}

			ShiftRowsInverse();
			SubBytesInverse();
			AddRoundKey(0);
			// 마지막 10라운드는 MixColumns를 하지 않습니다

			for (int i = 0; i < 4; i++) {
				for (int j = 0; j < 4; j++) {
					plain[i * 4 + j] = state[j][i];
				}
			}	// 각 state의 값을 순서에 맞게 plain 배열에 넣어줍니다

			fwrite(plain, sizeof(char), SIZE, plain2File);
			// 암호문을 파일에 적어줍니다

			readChar = fread(cipher, sizeof(char), SIZE, cipherFile); // 다음 루프
		}

		cout << "Decryption using AES method is completed." << endl
			<< "The output file is plain2.bin" << endl;

		return 0;	// 프로그램 종료
	}

	else if (control == 'h') {
	cout << "This is a modified AES system." << endl
		<< "The equation for AES used by default is x^8+x^4+x^3+x+1" << endl
		<< "However, the equation used for implementation in this program is x^8+x^7+x^6+x^5+x^2+x+1" << endl
		<< "Using plain.bin and key.bin to generate cipher.bin is encoding." << endl
		<< "Using cipher.bin and key.bin to generate plain2.bin is decoding. " << endl;
	}
}