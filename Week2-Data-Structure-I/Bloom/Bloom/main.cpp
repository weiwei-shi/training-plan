#include <iostream>
#include "BloomFilter.h"
#include "hash.h"
#include <string> 
#include<windows.h>
#include<math.h>
using namespace std;

int main(){
    bloomfilter bloom;
    int a, number = 0;
    string s;
    double time = 0;
    LARGE_INTEGER nFreq;
    LARGE_INTEGER nBeginTime;
    LARGE_INTEGER nEndTime;
    cout << "----------------����-----------------" << endl;
    cout << "�������������" << endl;
    //����30000������
    QueryPerformanceFrequency(&nFreq);
    QueryPerformanceCounter(&nBeginTime);//��ʼ��ʱ 
    for (int i = 0; i < 30000; i++) {
        s = to_string(i);
        bloom.add(s);
    }
    QueryPerformanceCounter(&nEndTime);//ֹͣ��ʱ  
    time = (double)(nEndTime.QuadPart - nBeginTime.QuadPart) / (double)nFreq.QuadPart;//�������ִ��ʱ�䵥λΪs  
    cout << "���������ʱ�䣺" << time * 1000 << "ms" << endl;
    //��ѯ����������ڵ���
    for (int i = 30000; i < 60000; i++) {
        s = to_string(i);
        if (bloom.isContain(s)) {
            number++;
        }
    }
    cout << "������" << number << endl;
    cout << "���ʣ�" << (double)number/30000 << endl;
    cout << "��������ʣ�" << exp(-BIT_SIZE * log(2) * log(2) / 30000) << endl << endl;
    cout << "�������������" << endl;
    //����50000������
    number = 0;
    QueryPerformanceFrequency(&nFreq);
    QueryPerformanceCounter(&nBeginTime);//��ʼ��ʱ 
    for (int i = 0; i < 50000; i++) {
        s = to_string(i);
        bloom.add(s);
    }
    QueryPerformanceCounter(&nEndTime);//ֹͣ��ʱ  
    time = (double)(nEndTime.QuadPart - nBeginTime.QuadPart) / (double)nFreq.QuadPart;//�������ִ��ʱ�䵥λΪs  
    cout << "���������ʱ�䣺" << time * 1000 << "ms" << endl;
    //��ѯ����������ڵ���
    for (int i = 50000; i < 100000; i++) {
        s = to_string(i);
        if (bloom.isContain(s)) {
            number++;
        }
    }
    cout << "������" << number << endl;
    cout << "���ʣ�" << (double)number / 50000 << endl;
    cout << "��������ʣ�" << exp(-BIT_SIZE * log(2) * log(2) / 50000) << endl << endl;
    cout << "��ѡ�������1.���� 2.��ѯ �������˳�" << endl;
    cin >> a;
    while (a > 0 && a < 3) {
        if (a == 1) {
            cout << "������Ҫ������ַ���" << endl;
            cin >> s;
            bloom.add(s);
            cout << "����ɹ�" << endl;
        }
        else if (a == 2) {
            cout << "������Ҫ��ѯ���ַ���" << endl;
            cin >> s;
            if(bloom.isContain(s))
                cout << "��ѯ�ɹ�" << endl;
            else
                cout << "��ѯʧ��" << endl;
        }
        else {
            cout << "����������1��������2" << endl;
        }
        cout << "��ѡ�������1.���� 2.��ѯ �������˳�" << endl;
        cin >> a;
    }
}