// (C) Michael Chourdakis
#pragma once


	// ---------------------------------------------------------------------
	// RKEY, quick registry access 
	class RKEY
		{
		private:
			HKEY k = 0;
		public:


			class VALUE
				{
				public:
					std::wstring name;
					std::vector<char> value; // For enums
					HKEY k = 0;
					mutable DWORD ty = 0;

					VALUE(const wchar_t* s, HKEY kk)
						{
						if (s)
							name = s;
						k = kk;
						}

					bool operator =(const wchar_t* val)
						{
						ty = REG_SZ;
						if (RegSetValueEx(k, name.c_str(), 0, REG_SZ, (BYTE*)val, (DWORD)(wcslen(val)*sizeof(wchar_t))) == ERROR_SUCCESS)
							return true;
						return false;
						}
					bool operator =(unsigned long val)
						{
						ty = REG_DWORD;
						return RegSetValueEx(k, name.c_str(), 0, REG_DWORD, (BYTE*)&val, sizeof(val)) == ERROR_SUCCESS;
						}
					bool operator =(unsigned long long val)
						{
						ty = REG_QWORD;
						return RegSetValueEx(k, name.c_str(), 0, REG_QWORD, (BYTE*)&val, sizeof(val)) == ERROR_SUCCESS;
						}


					bool Exists()
						{
						DWORD ch = 0;
						if (RegQueryValueEx(k, name.c_str(), 0, &ty, 0, &ch) == ERROR_FILE_NOT_FOUND)
							return false;
						return true;
						}

					template <typename T>
					operator T() const
						{
						T ch = 0;
						RegQueryValueEx(k, name.c_str(), 0, &ty, 0, &ch);
						std::vector<char> d(ch + 10);
						ch += 10;
						RegQueryValueEx(k, name.c_str(), 0, &ty, (LPBYTE)d.data(), &ch);
						T ret = 0;
						memcpy(&ret, d.data(), sizeof(T));
						return ret;
						}

					operator std::wstring() const
						{
						DWORD ch = 0;
						RegQueryValueEx(k, name.c_str(), 0, &ty, 0, &ch);
						std::vector<char> d(ch + 10);
						ch += 10;
						RegQueryValueEx(k, name.c_str(), 0, &ty, (LPBYTE)d.data(), &ch);
						return std::wstring((const wchar_t*)d.data());
						}

					bool Delete()
						{
						return (RegDeleteValue(k, name.c_str()) == ERROR_SUCCESS);
						}




				};



			RKEY(HKEY kk)
				{
				k = kk;
				}


			RKEY(const RKEY& k)
				{
				operator =(k);
				}
			void operator =(const RKEY& r)
				{
				Close();
				DuplicateHandle(GetCurrentProcess(), r.k, GetCurrentProcess(), (LPHANDLE)&k, 0, false, DUPLICATE_SAME_ACCESS);
				}
			void operator =(const wchar_t* v)
			{
				RegSetValue(k, 0, REG_SZ, (LPCWSTR)v,(DWORD) wcslen(v)*2);
			}

			RKEY(RKEY&& k)
				{
				operator =(std::forward<RKEY>(k));
				}
			void operator =(RKEY&& r)
				{
				Close();
				k = r.k;
				r.k = 0;
				}

			void operator =(HKEY kk)
				{
				Close();
				k = kk;
				}

			RKEY(HKEY root, const wchar_t* subkey, DWORD acc = KEY_ALL_ACCESS, bool Op = false)
				{
				Load(root, subkey, acc,Op);
				}
			bool Load(HKEY root, const wchar_t* subkey, DWORD acc = KEY_ALL_ACCESS,bool Op = false)
				{
				Close();
				if (Op)
					return (RegOpenKeyEx(root, subkey, 0, acc, &k) == ERROR_SUCCESS);
				return (RegCreateKeyEx(root, subkey, 0, 0, 0, acc, 0, &k, 0) == ERROR_SUCCESS);
				}

			void Close()
				{
				if (k)
					RegCloseKey(k);
				k = 0;
				}

			~RKEY()
				{
				Close();
				}

			bool Valid() const
				{
				if (k)
					return true;
				return false;
				}

			bool DeleteSingle(const wchar_t* sub)
				{
				return (RegDeleteKey(k, sub) == ERROR_SUCCESS);
				}

			bool Delete(const wchar_t* sub = 0)
				{
#if _WIN32_WINNT >= 0x600
				return (RegDeleteTree(k, sub) == ERROR_SUCCESS);
#else
				return false;
#endif
				}

			bool Flush()
				{
				return (RegFlushKey(k) == ERROR_SUCCESS);
				}

			std::vector<std::wstring> EnumSubkeys() const
				{
				std::vector<std::wstring> data;
				for (int i = 0;; i++)
					{
					std::vector<wchar_t> n(300);
					DWORD sz = (DWORD)n.size();
					if (RegEnumKeyEx(k, i, n.data(), &sz, 0, 0, 0, 0) != ERROR_SUCCESS)
						break;
					data.push_back(n.data());
					}
				return data;
				}

			std::vector<VALUE> EnumValues() const
				{
				std::vector<VALUE> data;
				for (int i = 0;; i++)
					{
					std::vector<wchar_t> n(300);
					DWORD sz = (DWORD)n.size();
					DWORD ay = 0;
					RegEnumValue(k, i, n.data(), &sz, 0, 0, 0, &ay);
					std::vector<char> v(ay);
					DWORD ty = 0;
					sz = (DWORD)n.size();
					if (RegEnumValue(k, i, n.data(), &sz, 0, &ty, (LPBYTE)v.data(), &ay) != ERROR_SUCCESS)
						break;

					VALUE x(n.data(), k);
					x.ty = ty;
					x.value = v;
					data.push_back(x);
					}
				return data;
				}

			VALUE operator [](const wchar_t* v) const
				{
				VALUE kv(v, k);
				return kv;
				}

			operator HKEY()
				{
				return k;
				}
		};

