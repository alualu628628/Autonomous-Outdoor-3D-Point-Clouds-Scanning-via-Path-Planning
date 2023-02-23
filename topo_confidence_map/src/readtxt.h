#ifndef READTXT_H
#define READTXT_H
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>//for istringstream function
using namespace std;
/*================================================
read orignal matrix
=================================================*/
void ReadMatrix(string fileName,std::vector<std::vector<double>> & feamatrix,int n=1);
//�Թ켣�߽��г�������
//ʵ���У����ڹ켣�߼�¼����ÿ5�����λ�ã����������ٶ�30km/h����8��/�룬һ��125���뼴25�μ��
void Sampling(std::vector<std::vector<double>> & feamatrix,int n=25);
/*================================================

=================================================*/
class Dividefeandclass{
public:
	//Classification at the last position 
	void Extlabelatend(std::vector<std::vector<double>> & , std::vector<int> &  );
	//Classification at the first position 
	void Extlabelatbegin(std::vector<std::vector<double>> &, std::vector<int> &  );
	void Extlabelatbegin(std::vector<std::vector<double>> &, std::vector<double> &  );
	//delete some feature
	void Removesomefeature(std::vector<std::vector<double>> &, std::vector<int> & );
};

#endif