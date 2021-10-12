
/**
 * This file is part of Nokia OMAF implementation
 *
 * Copyright (c) 2018-2021 Nokia Corporation and/or its subsidiary(-ies). All rights reserved.
 *
 * Contact: omaf@nokia.com
 *
 * This software, including documentation, is protected by copyright controlled by Nokia Corporation and/ or its
 * subsidiaries. All rights are reserved.
 *
 * Copying, including reproducing, storing, adapting or translating, any or all of this material requires the prior
 * written consent of Nokia.
 */
#ifndef ISOBMFF_UNIONHELPERS_HPP
#define ISOBMFF_UNIONHELPERS_HPP

namespace ISOBMFF {
    namespace UnionHelpers
    {
        template <typename... Types>
        struct UnionStorage
        {
            enum { NumFields = 0 };
        };

        template <typename T, typename... Types>
        struct UnionStorage<T, Types...>
        {
            using type = T;
            using tail_type = UnionStorage<Types...>;
            enum { NumFields = 1 +  UnionStorage<Types...>::NumFields };
            union {
                typename std::aligned_storage<sizeof(T), alignof(T)>::type head;
                tail_type tail;
            };
        };

        template <typename BaseStorage, typename Storage, size_t Offset, size_t Index, typename... Types>
        struct UnionFieldAux
        {
        };

        template <typename BaseStorage, typename Storage, size_t Offset, size_t Index, typename T, typename... Types>
        struct UnionFieldAux<BaseStorage, Storage, Offset, Index, T, Types...>
            : UnionFieldAux<BaseStorage, typename Storage::tail_type, Offset + offsetof(Storage, tail), Index - 1, Types...>
        {
        };

        template <typename BaseStorage, typename Storage, size_t Offset, typename T, typename... Types>
        struct UnionFieldAux<BaseStorage, Storage, Offset, 0, T, Types...>
        {
            using type = typename Storage::type;

            static void* storage(BaseStorage& aBaseStorage)
            {
                return &aBaseStorage.head + Offset;
            }

            static const void* storage(const BaseStorage& aBaseStorage)
            {
                return &aBaseStorage.head + Offset;
            }

            static type& get(BaseStorage& aBaseStorage)
            {
                return *reinterpret_cast<type*>(storage(aBaseStorage));
            }

            static const type& get(const BaseStorage& aBaseStorage)
            {
                return *reinterpret_cast<const type*>(storage(aBaseStorage));
            }
        };

        template <typename Storage, size_t Index, typename... Types>
        struct UnionField : UnionFieldAux<Storage, Storage, 0u, size_t(Index), Types...>
        {
        };

        template <typename BaseStorage, typename Storage, size_t Offset, size_t ScanIndex, typename... Types>
        struct UnionFieldDynamicAux
        {
            static
            void destruct(BaseStorage& /* aBaseStorage */, size_t /* Index */)
            {
                assert(false);  // invalid index
            }

            static
            void copyConstruct(const BaseStorage& /* aSourceStorage */, BaseStorage& /* aDestStorage */, size_t /* aIndex */)
            {
                assert(false);  // invalid index
            }

            static
            void moveConstruct(const BaseStorage& /* aSourceStorage */, BaseStorage& /* aDestStorage */, size_t /* aIndex */)
            {
                assert(false);  // invalid index
            }

            static
            void assign(const BaseStorage& /* aSourceStorage */, BaseStorage& /* aDestStorage */, size_t /* aIndex */)
            {
                assert(false);  // invalid index
            }

            static
            void move(const BaseStorage& /* aSourceStorage */, BaseStorage& /* aDestStorage */, size_t /* aIndex */)
            {
                assert(false);  // invalid index
            }

            static int compare(const BaseStorage& /* aAStorage */, const BaseStorage& /* aBStorage */, size_t /* aIndex */)
            {
                assert(false);  // invalid index
                return 0;
            }
        };

        template <typename BaseStorage, size_t Offset>
        struct UnionStorageHelper
        {
            static void* storage(BaseStorage& aBaseStorage)
            {
                return &aBaseStorage.head + Offset;
            }

            static const void* storage(const BaseStorage& aBaseStorage)
            {
                return &aBaseStorage.head + Offset;
            }
        };

        template <typename BaseStorage,
                  typename Storage,
                  size_t Offset,
                  size_t ScanIndex,
                  typename T,
                  typename... Types>
        struct UnionFieldDynamicAux<BaseStorage, Storage, Offset, ScanIndex, T, Types...>
            : UnionStorageHelper<BaseStorage, Offset>
        {
            using Helper = UnionStorageHelper<BaseStorage, Offset>;
            using type = typename Storage::type;

            static int compare(const BaseStorage& aAStorage, const BaseStorage& aBStorage, size_t aIndex)
            {
                if (aIndex == ScanIndex)
                {
                    auto& src = *reinterpret_cast<const type*>(Helper::storage(aAStorage));
                    auto& dst = *reinterpret_cast<const type*>(Helper::storage(aBStorage));
                    if (src == dst)
                    {
                        return 0;
                    }
                    else if (src < dst)
                    {
                        return -1;
                    }
                    else /* if (src > dst) */
                    {
                        return 1;
                    }
                }
                else
                {
                    return UnionFieldDynamicAux<BaseStorage, typename Storage::tail_type,
                                                Offset + offsetof(Storage, tail), ScanIndex + 1, Types...>()
                        .compare(aAStorage, aBStorage, aIndex);
                }
            }

            static void destruct(BaseStorage& aBaseStorage, size_t aIndex)
            {
                if (aIndex == ScanIndex)
                {
                    auto value = reinterpret_cast<type*>(Helper::storage(aBaseStorage));
                    value->~type();
                }
                else
                {
                    UnionFieldDynamicAux<BaseStorage, typename Storage::tail_type, Offset + offsetof(Storage, tail),
                                         ScanIndex + 1, Types...>()
                        .destruct(aBaseStorage, aIndex);
                }
            }

            static void assign(const BaseStorage& aSourceStorage, BaseStorage& aDestStorage, size_t aIndex)
            {
                if (aIndex == ScanIndex)
                {
                    auto src = reinterpret_cast<const type*>(Helper::storage(aSourceStorage));
                    auto dst = reinterpret_cast<type*>(Helper::storage(aDestStorage));
                    *dst     = *src;
                }
                else
                {
                    UnionFieldDynamicAux<BaseStorage, typename Storage::tail_type, Offset + offsetof(Storage, tail),
                                         ScanIndex + 1, Types...>()
                        .assign(aSourceStorage, aDestStorage, aIndex);
                }
            }

            static void move(BaseStorage& aSourceStorage, BaseStorage& aDestStorage, size_t aIndex)
            {
                if (aIndex == ScanIndex)
                {
                    auto src = reinterpret_cast<type*>(Helper::storage(aSourceStorage));
                    auto dst = reinterpret_cast<type*>(Helper::storage(aDestStorage));
                    *dst     = std::move(*src);
                }
                else
                {
                    UnionFieldDynamicAux<BaseStorage, typename Storage::tail_type, Offset + offsetof(Storage, tail),
                                         ScanIndex + 1, Types...>()
                        .move(aSourceStorage, aDestStorage, aIndex);
                }
            }

            static void copyConstruct(const BaseStorage& aSourceStorage, BaseStorage& aDestStorage, size_t aIndex)
            {
                if (aIndex == ScanIndex)
                {
                    auto src = reinterpret_cast<const type*>(Helper::storage(aSourceStorage));
                    auto dst = reinterpret_cast<type*>(Helper::storage(aDestStorage));
                    new (dst) type(*src);
                }
                else
                {
                    UnionFieldDynamicAux<BaseStorage, typename Storage::tail_type, Offset + offsetof(Storage, tail),
                                         ScanIndex + 1, Types...>()
                        .copyConstruct(aSourceStorage, aDestStorage, aIndex);
                }
            }

            static void moveConstruct(BaseStorage& aSourceStorage, BaseStorage& aDestStorage, size_t aIndex)
            {
                if (aIndex == ScanIndex)
                {
                    auto src = reinterpret_cast<type*>(Helper::storage(aSourceStorage));
                    auto dst = reinterpret_cast<type*>(Helper::storage(aDestStorage));
                    new (dst) type(std::move(*src));
                }
                else
                {
                    UnionFieldDynamicAux<BaseStorage, typename Storage::tail_type, Offset + offsetof(Storage, tail),
                                         ScanIndex + 1, Types...>()
                        .moveConstruct(aSourceStorage, aDestStorage, aIndex);
                }
            }
        };

        template <typename Storage, typename... Types>
        struct UnionFieldDynamic : UnionFieldDynamicAux<Storage, Storage, 0u, 0u, Types...>
        {
        };

        template <typename BaseStorage, typename Storage, size_t Offset, size_t ScanIndex, typename BaseType, typename... Types>
        struct UnionGetBaseAux
        {
            static
            BaseType& base(BaseStorage& /* aStorage */, size_t /* aIndex */)
            {
                assert(false);  // invalid index
                // for MSVC
                return *static_cast<BaseType*>(nullptr);
            }

            static
            const BaseType& base(const BaseStorage& /* aStorage */, size_t /* aIndex */)
            {
                assert(false);  // invalid index
                // for MSVC
                return *static_cast<const BaseType*>(nullptr);
            }
        };

        template <typename BaseStorage,
                  typename Storage,
                  size_t Offset,
                  size_t ScanIndex,
                  typename BaseType,
                  typename T,
                  typename... Types>
        struct UnionGetBaseAux<BaseStorage, Storage, Offset, ScanIndex, BaseType, T, Types...>
            : public UnionStorageHelper<BaseStorage, Offset>
        {
            using Helper = UnionStorageHelper<BaseStorage, Offset>;

            static BaseType& base(BaseStorage& aStorage, size_t aIndex)
            {
                if (aIndex == ScanIndex)
                {
                    return *reinterpret_cast<BaseType*>(Helper::storage(aStorage));
                }
                else
                {
                    return UnionGetBaseAux<BaseStorage, typename Storage::tail_type, Offset + offsetof(Storage, tail),
                                           ScanIndex + 1, BaseType, Types...>()
                        .base(aStorage, aIndex);
                }
            }

            static const BaseType& base(const BaseStorage& aStorage, size_t aIndex)
            {
                if (aIndex == ScanIndex)
                {
                    return *reinterpret_cast<const BaseType*>(Helper::storage(aStorage));
                }
                else
                {
                    return UnionGetBaseAux<BaseStorage, typename Storage::tail_type, Offset + offsetof(Storage, tail),
                                           ScanIndex + 1, BaseType, Types...>()
                        .base(aStorage, aIndex);
                }
            }
        };

        template <typename Storage, typename BaseType, typename... Types>
        struct UnionGetBase : UnionGetBaseAux<Storage, Storage, 0u, 0u, BaseType, Types...>
        {
        };
    }  // namespace UnionHelpers
}

#endif // ISOBMFF_UNIONHELPERS_HPP
