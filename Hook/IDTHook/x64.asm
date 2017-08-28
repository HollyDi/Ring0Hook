
EXTERN ShowInformation:PROC
EXTERN OldKiSystemService:PROC

.CODE


FakeKiSystemService PROC
	call ShowInformation
	jmp OldKiSystemService
FakeKiSystemService ENDP

END