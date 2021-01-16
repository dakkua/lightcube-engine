/**
 * Copyright (C) Daniel Guerra Gallardo 
 *
 * @section LICENSE MIT License (Lee el archivo LICENSE para mas informaci�n)
 *
 * @section DESCRIPTION
 * M�dulo de contenedores, tipos complejos generales, algoritmos... Utilidades varias para LightCube Engine
 *
 * @author Daniel Guerra Gallardo
 * Contact: daniel.guerra.gallardo99@gmail.com
 */

#pragma once

#include <type_traits>
#include <memory>
#include <chrono>
#include <iostream>
#include <cstddef>
#include <fstream>
#include <internal/utf8.h>

using namespace std;

namespace LC_Utils
{
    
    /// <summary>
    /// Macros definidas a modo de depuraci�n en consola en modo debug. Sintaxis inspirada en Unity Engine.
    /// </summary>
    #ifndef STD_OUTPUT_HANDLE
    #define STD_OUTPUT_HANDLE
    #endif // !STD_OUTPUT_HANDLE

    #ifdef LC_DEBUG
    #define DEBUG_LOG(x) std::cout << x 
    #define N_DEBUG_LOG(x) std::cout << x << std::endl

    #define DEBUG_WARNING(x) std::cout << "\x1B[33m" << x << "\033[0m"
    #define N_DEBUG_WARNING(x) std::cout << "\x1B[33m" << x << "\033[0m" << std::endl

    #define DEBUG_ERROR(x) std::cout << "\x1B[31m" << x << "\033[0m" 
    #define N_DEBUG_ERROR(x) std::cout << "\x1B[31m" << x << "\033[0m" << std::endl
    #else
    #define DEBUG_LOG(x)
    #define N_DEBUG_LOG(x)

    #define DEBUG_WARNING(x)
    #define N_DEBUG_WARNING(x)

    #define DEBUG_ERROR(x)
    #define N_DEBUG_ERROR(x)
    #endif // LC_DEBUG

    // Clases: ObjectPool & ExecutionTimer
    // Code Blacksmith
    // https://twitter.com/codeblacksmith

     // Clases: Asset
    // Angel Rodrigez
    // angel.rodriguez@esne.edu

    /// <summary>
    /// Object Pool: Sistema que me ayuda a optimizar la instanciaci�n / destrucci�n de objetos en particiones, 
    /// en vez de alojarlas en la HEAP de la memoria y probocar un 'memory leak'. Util para uso de part�culas o cualquier otro elementos
    /// donde necesitemos instanciar muchos objetos al mismo tiempo, lo que obligar�a en un caso normal a reservar memoria individualmente
    /// y consumiendo demasiados recursos. Haciendo uso de un object pool, logramos reducir el tiempo en esto notablemente.
    /// </summary>
    /// <typeparam name="T"> El objeto que se le pasa</typeparam>
    /// <typeparam name="size"> Tama�o de la piscina de objetos</typeparam>
    template<class T, size_t size = 2048>
    class ObjectPool final
    {
    public:
        const static std::size_t Size = size;

        using value_type = T;
        using pointer = value_type*;

        ObjectPool()
        {
            for (auto i = 1; i < Size; ++i)
                mPool[i - 1].mNext = &mPool[i];

            mNextFree = &mPool[0];
        }

        ObjectPool(const ObjectPool&) = delete;

        ObjectPool(ObjectPool&& other) noexcept
            : mPool{ std::move(other.mPool) }
            , mNextFree{ other.mNextFree }
        {
            other.mNextFree = nullptr;
        }

        ~ObjectPool() = default;

        [[nodiscard]] pointer allocate()
        {
            if (mNextFree == nullptr)
                throw std::bad_alloc{};

            const auto item = mNextFree;
            mNextFree = item->mNext;

            return reinterpret_cast<pointer>(&item->mStorage);
        }

        void deallocate(pointer p) noexcept
        {
            const auto item = reinterpret_cast<Item*>(p);

            item->mNext = mNextFree;
            mNextFree = item;
        }

        template<class ...Args>
        [[nodiscard]] pointer construct(Args&& ...args)
        {
            return new (allocate()) value_type(std::forward<Args>(args)...);
        }

        void destroy(pointer p) noexcept
        {
            if (p == nullptr)
                return;

            p->~value_type();
            deallocate(p);
        }

        ObjectPool& operator =(const ObjectPool&) = delete;

        ObjectPool& operator =(ObjectPool&& other) noexcept
        {
            if (this == &other)
                return *this;

            mPool = std::move(other.mPool);
            mNextFree = other.mNextFree;

            other.mNextFree = nullptr;

            return *this;
        }

    private:
        union Item
        {
            std::aligned_storage_t<sizeof(value_type), alignof(value_type)> mStorage;
            Item* mNext;
        };

        std::unique_ptr<Item[]> mPool = std::make_unique<Item[]>(Size);
        Item* mNextFree = nullptr;
    };

    /// <summary>
    /// Cron�metro que utiliza la libreria standar de C++ y es capaz de medir en milisegundos en ciclos de reloj
    /// </summary>
    /// <typeparam name="Resolution">unidad de medida de la clase</typeparam>
    template<class Resolution = std::chrono::milliseconds>
    class ExecutionTimer final
    {
    public:
        using Clock = std::conditional_t<std::chrono::high_resolution_clock::is_steady,
            std::chrono::high_resolution_clock,
            std::chrono::steady_clock>;

        ExecutionTimer() = default;
        ~ExecutionTimer() = default;

        void endTimer()
        {
            DEBUG_LOG("Crono Elapsed Time: ");
            DEBUG_LOG(std::chrono::duration_cast<Resolution>(Clock::now() - mStart).count());
            DEBUG_LOG("\n");
        };

    private:
        Clock::time_point mStart = Clock::now();
    };

    /// <summary>
    /// Clase de lectura / escritura de archivos de texto
    /// </summary>
    class Serialization
    {
    private:
        
        std::string file_name;
        std::string path = "../../resources/";    
        std::string resource;

    public:

        Serialization() = default;
        Serialization(std::string _file_name)
        {
            file_name = _file_name;
            resource = path + _file_name;
            //std::ifstream file(_fileName, std::ifstream::binary);
        }
        ~Serialization() = default;

    public:

        void read()
        {
            N_DEBUG_WARNING(resource);

            std::ifstream file(file_name, std::ifstream::binary);

            if (file)
            {
                file.seekg(0, ifstream::end);

                if (file.good())
                {
                    int length = file.tellg();

                    if (length > 0)
                    {
                        file.seekg(0, ifstream::beg);

                        char* buffer = new char[length];

                        if (file.good())
                        {
                            file.read(buffer, length);

                            if (file.good())
                            {
                                file.close();

                                DEBUG_LOG("Se ha leido correctamente el archivo: ");
                                N_DEBUG_LOG(file_name);
                            }
                            else
                            {
                                N_DEBUG_ERROR("No se ha podido leer el archivo");
                            }
                        } 

                        delete[] buffer;
                    }              
                }          
            }  
        }

        void write(string out)
        {
            N_DEBUG_WARNING(resource);

            std::ofstream file(file_name, std::ifstream::binary);

            if (file.is_open())
            {
                file << out;

                if (file.good())
                {
                    file.close();

                    DEBUG_LOG("Se ha escrito correctamente el archivo: ");
                    N_DEBUG_LOG(file_name);
                }
                else
                {
                    N_DEBUG_ERROR("No se ha podido escribir el archivo");
                }              
            }
        }

        void open()
        {
            N_DEBUG_WARNING(resource);

            std::fstream file;

            file.open("test.txt");

            if (file.is_open())
            {
                std::string item;

                while (!file.eof())
                {
                    file >> item;
                }

                if (file.good())
                {
                    file.close();

                    DEBUG_LOG("Se ha abierto correctamente el archivo: ");
                    DEBUG_LOG(file_name);
                }
                else
                {
                    N_DEBUG_ERROR("No se ha podido abrir el archivo");
                }
            }
        }  
    };

    /// <summary>
    /// Clase Serialization encapsulada con capacidad de multi-motor
    /// </summary>
    class Asset
    {

        std::ifstream reader;

    public:

        Asset(const string& path) : reader(path)
        {
        }

        byte get()
        {
            return byte(reader.get());
        }

        size_t read(byte* buffer, size_t amount)
        {
            reader.read(reinterpret_cast<char*>(buffer), amount);

            return reader.gcount();
        }

        bool good() { return reader.good(); }
        bool bad() { return reader.bad(); }
        bool fail() { return reader.fail(); }
        bool eof() { return reader.eof(); }

    };
}

