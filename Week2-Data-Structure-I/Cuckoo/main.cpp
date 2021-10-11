#include <iostream>
#include "CuckooFilter.h"
#include <string>
#include<windows.h>
using namespace std;

int main() {
    cuckoofilter cuckoo;
    int a, number = 0;
    string s;
    double time = 0;
    LARGE_INTEGER nFreq;
    LARGE_INTEGER nBeginTime;
    LARGE_INTEGER nEndTime;
    cout << "---------------------����-------------------" << endl;
    cout << "�������������" << endl;
    //����30000������
    QueryPerformanceFrequency(&nFreq);
    QueryPerformanceCounter(&nBeginTime);//��ʼ��ʱ 
    for (int i = 0; i < 30000; i++) {
        s = to_string(i);
        cuckoo.add(s);
    }
    QueryPerformanceCounter(&nEndTime);//ֹͣ��ʱ  
    time = (double)(nEndTime.QuadPart - nBeginTime.QuadPart) / (double)nFreq.QuadPart;//�������ִ��ʱ�䵥λΪs  
    cout << "���������ʱ�䣺" << time * 1000 << "ms" << endl;
    //��ѯ����������ڵ���
    for (int i = 30000; i < 60000; i++) {
        s = to_string(i);
        if (cuckoo.isContain(s)) {
            number++;
        }
    }
    cout << "������" << number << endl;
    cout << "���ʣ�" << (double)number / 30000 << endl << endl;
    cout << "�������������" << endl;
    //����50000������
    QueryPerformanceFrequency(&nFreq);
    QueryPerformanceCounter(&nBeginTime);//��ʼ��ʱ 
    for (int i = 0; i < 50000; i++) {
        s = to_string(i);
        cuckoo.add(s);
    }
    QueryPerformanceCounter(&nEndTime);//ֹͣ��ʱ  
    time = (double)(nEndTime.QuadPart - nBeginTime.QuadPart) / (double)nFreq.QuadPart;//�������ִ��ʱ�䵥λΪs  
    cout << "���������ʱ�䣺" << time * 1000 << "ms" << endl;
    //��ѯ����������ڵ���
    for (int i = 50000; i < 100000; i++) {
        s = to_string(i);
        if (cuckoo.isContain(s)) {
            number++;
        }
    }
    cout << "������" << number << endl;
    cout << "���ʣ�" << (double)number / 50000 << endl << endl;
    cout << "��ѡ�������1.���� 2.��ѯ 3.ɾ�� �������˳�" << endl;
    cin >> a;
    while (a > 0 && a < 4) {
        if (a == 1) {
            cout << "������Ҫ������ַ���" << endl;
            cin >> s;
            if (cuckoo.add(s))
                cout << "����ɹ�" << endl;
            else
                cout << "����ʧ��" << endl;
        }
        else if (a == 2) {
            cout << "������Ҫ��ѯ���ַ���" << endl;
            cin >> s;
            if (cuckoo.isContain(s))
                cout << "��ѯ�ɹ�" << endl;
            else
                cout << "��ѯʧ��" << endl;
        }
        else if (a == 3) {
            cout << "������Ҫɾ�����ַ���" << endl;
            cin >> s;
            if (cuckoo.Delete(s))
                cout << "ɾ���ɹ�" << endl;
            else
                cout << "ɾ��ʧ��" << endl;
        }
        else {
            cout << "����������1��2��3" << endl;
        }
        cout << "��ѡ�������1.���� 2.��ѯ 3.ɾ�� �������˳�" << endl;
        cin >> a;
    }
}