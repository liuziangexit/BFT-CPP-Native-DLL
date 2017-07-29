#pragma once
#include <string>
#include "DC_ERROR.h"
//Version 2.4.1V8
//20170317

#define ERROR_CANTOPENFILE DC::DC_ERROR("CAN NOT OPEN FILE", -1)

namespace DC {

	namespace File {
		//ĳЩ������ptr���õ����ذ汾
		//��ʧ�����쳣

		class file_ptr final {//����C�����ļ�ָ���������ڵİ�װ��
			                  //����ͨ��if(file_ptr)�ķ�ʽ�ж�file_ptr�Ƿ��Ѿ���
			                  //������ʹ��ʱ�ֶ�fclose(file_ptr::get())���ⲻ�ᵼ��δ������Ϊ
			                  //��֧���ƶ�����
		public:
			file_ptr() :fp(nullptr) {}

			file_ptr(FILE* input) :fp(input) {}

			file_ptr(const file_ptr&) = delete;

			file_ptr(file_ptr&& input) {
				clear();
				this->fp = input.fp;
				input.fp = nullptr;
			}

			~file_ptr() {
				clear();
			}

			file_ptr& operator=(const file_ptr&) = delete;

			file_ptr& operator=(file_ptr&& input) {
				clear();
				this->fp = input.fp;
				input.fp = nullptr;
				return *this;
			}

			operator bool() {
				if (fp == NULL || fp == nullptr) return false;
				return true;
			}

			inline FILE* get() {
				return fp;
			}

			inline void reset(FILE* input) {
				clear();
				fp = input;
			}

		private:
			inline void clear() {
				if (fp == NULL || fp == nullptr) return;
				fclose(fp);
			}

		private:
			FILE *fp;
		};

		inline void del(const std::string& filename) {
			remove(filename.c_str());
		}
		
		inline bool exists(const std::string& filename) {
			file_ptr ptr(fopen(filename.c_str(), "r"));
			return (bool)ptr;
		}

		inline bool exists(const std::string& filename, file_ptr& inputptr) {//ͬʱ�ж��Ƿ���ں��Ƿ��ܴ�
			file_ptr ptr(fopen(filename.c_str(), "r"));
			inputptr = std::move(ptr);
			return (bool)inputptr;
		}

		std::size_t getSize(const std::string& filename) {//��ȡ�ļ�����
			file_ptr ptr;
			if (!exists(filename, ptr)) throw ERROR_CANTOPENFILE;
			fseek(ptr.get(), 0L, SEEK_END);
			return ftell(ptr.get());
		}

		std::string read(const std::string& filename) {
			file_ptr ptr;
			if (!exists(filename, ptr)) throw ERROR_CANTOPENFILE;
			std::string returnvalue;
			auto&& ch = fgetc(ptr.get());
			while (ch != EOF) {
				returnvalue.push_back(ch);
				ch = fgetc(ptr.get());
			}
			return returnvalue;
		}

		bool write(const std::string& filename,const std::string& write) {//����д��
			file_ptr ptr(fopen(filename.c_str(), "w"));
			if (!ptr) throw ERROR_CANTOPENFILE;
			if (fprintf(ptr.get(), "%s", write.c_str()) == -1) return false;
			return true;
		}

		bool writeAppend(const std::string& filename, const std::string& write) {
			file_ptr ptr(fopen(filename.c_str(), "a"));
			if (!ptr) throw ERROR_CANTOPENFILE;
			if (fprintf(ptr.get(), "%s", write.c_str()) == -1) return false;
			return true;
		}

	}

}