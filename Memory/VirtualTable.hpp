#pragma once
#include <Windows.h>
#include <memory>
#include <cstdlib>

namespace Hooking
{
	class VirtualTable
	{
	public:
		template <typename Type>
		bool VMT( void* Address , void* Function , int Index , Type* Original )
		{
			this->OriginalVTable = *( uintptr_t** )( Address );

			if ( this->LastHookedFunctionAddress && this->LastHookedFunctionIndex )
			{
				if ( this->LastHookedFunctionAddress == this->OriginalVTable[ this->LastHookedFunctionIndex ] )
				{
					return false;
				}
			}

			void* OriginalFunction = reinterpret_cast< void* >( this->OriginalVTable[ Index ] );
			if ( OriginalFunction != Function )
			{
				int AllocationSize = 0;
				while ( this->OriginalVTable[ AllocationSize ] )
				{
					++AllocationSize;
				}

				if ( Index < AllocationSize )
				{
					uintptr_t* AllocatedVTable = reinterpret_cast< uintptr_t* >( malloc( AllocationSize * sizeof( uintptr_t ) ) );

					memcpy( AllocatedVTable , this->OriginalVTable , AllocationSize * sizeof( uintptr_t ) );

					*Original = reinterpret_cast< Type >( OriginalFunction );

					AllocatedVTable[ Index ] = reinterpret_cast< uintptr_t >( Function );

					*( uintptr_t** )( Address ) = AllocatedVTable;

					this->LastHookedFunctionAddress = reinterpret_cast< uintptr_t >( Function );
					this->LastHookedFunctionIndex = Index;

					return true;
				}
			}

			return false;
		}

		template <typename Type>
		void RevertHook( Type* Original , void* Address )
		{
			if ( this->OriginalVTable && this->LastHookedFunctionIndex )
			{
				uintptr_t* VirtualTable = *( uintptr_t** ) ( Address );

				VirtualTable[ this->LastHookedFunctionIndex ] = reinterpret_cast< uintptr_t >( *Original );

				*( uintptr_t** ) Address = this->OriginalVTable;

				free( VirtualTable );

				this->LastHookedFunctionAddress = 0x0;
				this->LastHookedFunctionIndex = -1;
			}
		}

	private:
		uintptr_t* OriginalVTable;
		uintptr_t LastHookedFunctionAddress;
		int LastHookedFunctionIndex;
	};

	inline auto ProcessClientHook = std::make_unique< VirtualTable >( );
	inline auto ViewportClientHook = std::make_unique< VirtualTable >( );
}