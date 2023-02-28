
#pragma once

#include <memory>

namespace POC {
    template <class T, std::size_t growSize = 1024>
        class MemoryPool {
            /** 
             * the `Chunk` struct comes from: 
             * http://dmitrysoshnikov.com/compilers/writing-a-pool-allocator/
             */
            struct Chunk {  // this struct is pretty tricky
                /**
                 * When a chunk is free, the `next` contains the
                 * address of the next chunk in a list.
                 *
                 * When it's allocated, this space is used by
                 * the user.
                 */
                Chunk* next;
            };

            class Block {
                static const std::size_t chunkSize = sizeof(T) > sizeof(Chunk) ? sizeof(T) : sizeof(Chunk);
                uint8_t data[chunkSize * growSize];

                public:
                Block * const next;
                Block(Block *pnext): next(pnext) {  // we chain all the blocks by `next`
                    // optional: memset data
                }
                T* getChunk(std::size_t idx) {
                    return reinterpret_cast<T*>(&data[chunkSize * idx]);
                }
            };
            private:
            Chunk *firstFreeChunk = nullptr;
            Block *firstBlock = nullptr;
            std::size_t usedChunks = growSize;

            public:
            MemoryPool() = default;
            MemoryPool(MemoryPool && ) = delete;  // move ctor
            MemoryPool(const MemoryPool&) = delete;  // copy ctor
            MemoryPool operator=(const MemoryPool&&) = delete; // assignment operator
            MemoryPool operator=(const MemoryPool&) = delete; // assignment operator

            ~MemoryPool() {
                while (firstBlock) {  // delete all allocated blocks
                    Block *tmp = firstBlock->next;
                    delete firstBlock;
                    firstBlock = tmp;
                }
            }

            T *allocate() {
                /**
                 * `firstFreeChunk` will always be nullptr, unless `deallocate` is called
                 */
                if (firstFreeChunk) {
                    Chunk* p = firstFreeChunk;
                    firstFreeChunk= firstFreeChunk->next;
                    return reinterpret_cast<T*>(p);
                }

                /**
                 * usedChunks is initialized to be growSize,
                 * so when first allocation happens, a block will be newed.
                 */
                if (usedChunks >= growSize) {
                    /**
                     * single list will be constructed automatically
                     */
                    firstBlock = new Block(firstBlock);
                    usedChunks = 0;
                }

                return firstBlock->getChunk(usedChunks++);
            }

            void deallocate(T* p) {
                /**
                 * this is a little tricky,
                 * free chunk list is automatically constructed.
                 */
                Chunk *pchunk = reinterpret_cast<Chunk*>(p);
                pchunk->next = firstFreeChunk;
                firstFreeChunk = pchunk;
            }
        };

    template<class T, std::size_t growSize = 1024>
        class Allocator: private MemoryPool<T, growSize> {
        public:
            typedef T           value_type;
            typedef T*          pointer;
            typedef const T*    const_pointer;
            typedef T&          reference;
            typedef const T&    const_reference;
            typedef std::size_t size_type;
            // https://en.cppreference.com/w/cpp/types/ptrdiff_t
            typedef ptrdiff_t   difference_type;
            
            // rebind allocator of type U
            // https://stackoverflow.com/questions/14148756/what-does-template-rebind-do
            template <class U>
                struct rebind {
                    typedef Allocator<U, growSize> other;
                };

            /**
             * hint used for locality. ref.[Austern], p189
             * https://en.cppreference.com/w/cpp/memory/allocator/allocate
             * The pointer `hint` may be used to provide locality of reference: 
             *      the allocator, if supported by the implementation, 
             *      will attempt to allocate the new memory block as close as possible to hint.
             */
            pointer allocate(size_type n, const void* hint = 0) {
               if (n != 1 || hint) throw std::bad_alloc(); 
               return MemoryPool<T, growSize>::allocate();
            }

            void deallocate(pointer p, size_type n) {
                MemoryPool<T, growSize>::deallocate(p);
            }

            void construct(pointer p, const_reference val) {
                //https://stackoverflow.com/questions/222557/what-uses-are-there-for-placement-new
                //https://stackoverflow.com/questions/58900136/custom-allocator-including-placement-new-case
                new (p) T(val);  // placement new
            }

            void destroy(pointer p) {
                p->~T();
            }

    };
}

