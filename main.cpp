#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

// Semafory
sem_t przyszedl_nowy_klient;
sem_t bilet_do_salonu;
sem_t odczytany_bilet_do_salonu;
sem_t klient_obsluzony;

// Salon
struct salon {
  int id;
  int ilosc_miejsc;
  int zajete_miejsca;

  sem_t dostep_do_poczekalni;
  sem_t nowy_klient_w_poczekalni;
  sem_t wolny_golibroda;
};

// Zmienne
salon* wskazany_salon;
salon salony[3];


void* klientFun(void* t)
{
  pthread_t klient_id = pthread_self();

  // Poinformuj portiera o swoim przybyciu
  printf("Klient %u: Informuje portiera o swoim przybyciu\n", (unsigned)klient_id);
  sem_post(&przyszedl_nowy_klient);

  // Czekaj na informację do jakiego salonu masz się udać
  printf("Klient %u: Czeka na informacje do jakiego salonu sie udac\n", (unsigned)klient_id);
  sem_wait(&bilet_do_salonu);

  // Pobierz informacje o wskazanym salonie
  salon* moj_salon = wskazany_salon;

  // Potwierdź portierowi że się kierujesz do salonu
  printf("Klient %u: Udaje sie do wskazanego salonu o numerze %d\n", (unsigned)klient_id, (*moj_salon).id);
  sem_post(&odczytany_bilet_do_salonu);

  int i = 0;
  while(1)
  {
    // Czekaj na możliwość wejścia do poczekalni przed salonem
    printf("Klient %u: Czeka na mozliwosc wejscia do poczekalni\n", (unsigned)klient_id);
    sem_wait(&(*moj_salon).dostep_do_poczekalni);

    // Wejdź do poczekalni
    printf("Klient %u: Wchodzi do poczekalni\n", (unsigned)klient_id);

    // Sprawdza czy jest wolne miejsce w wskazanym salonie
    if ((*moj_salon).zajete_miejsca < (*moj_salon).ilosc_miejsc)
    {
      (*moj_salon).zajete_miejsca++;
      break;
    }
    else
    {
      // Chodź po salonach aż znajdziesz wolne miejsce
      moj_salon = &salony[i % 3];
      i++;
    }
  }

  // Poinformuj golibrodę o swoim przybyciu
  printf("Klient %u: Informuje golibrode o swoim przybyciu\n", (unsigned)klient_id);
  sem_post(&(*moj_salon).nowy_klient_w_poczekalni);

  // Zwalnia dostęp do poczekalni
  printf("Klient %u: Zwalnia dostęp do poczekalni\n", (unsigned)klient_id);
  sem_post(&(*moj_salon).dostep_do_poczekalni);

  // Czekaj na wolnego golibrodę
  printf("Klient %u: Czeka na wolnego golibrode\n", (unsigned)klient_id);
  sem_wait(&(*moj_salon).wolny_golibroda);

  // Golenie
  printf("Klient %u: Zostaje obsluzony przez golibrode i konczy swoje dzialanie\n", (unsigned)klient_id);

  sem_wait(&klient_obsluzony);
  int ilosc_obslugiwanych_klientow;
  sem_getvalue(&klient_obsluzony, &ilosc_obslugiwanych_klientow);
  printf("Klienci obługiwani: %d\n", (int)ilosc_obslugiwanych_klientow);

  pthread_exit(NULL);
}


void* portierFun(void* t)
{
  int ilosc_miejsc_w_salonach = 0;
  int ilosc_obsluzonych_klientow = 0;
  int modulo;
  int ilosc_obslugiwanych_klientow;

  // Wylicz ilość miejsc w salonach
  for (int i=0; i<3; i++)
    ilosc_miejsc_w_salonach += salony[i].ilosc_miejsc;
  ilosc_miejsc_w_salonach += 6; // Tyle mamy golibrod

  while(1)
  { 
    sem_getvalue(&klient_obsluzony, &ilosc_obslugiwanych_klientow);
    //printf("Klienci obługiwani: %d\n", (int)ilosc_obslugiwanych_klientow);

    if (ilosc_miejsc_w_salonach > ilosc_obslugiwanych_klientow)
    {
      // Czekaj na klienta
      printf("Portier: czeka na klienta...\n");
      sem_wait(&przyszedl_nowy_klient);

      // Wyslij informację że nowy klient będzie zaraz obsługiwany
      sem_post(&klient_obsluzony);
      sem_getvalue(&klient_obsluzony, &ilosc_obslugiwanych_klientow);
      printf("Klient obsluzony %d\n", (int)ilosc_obslugiwanych_klientow);

      // Oblicz salon do którego ma trafić klient
      modulo = ilosc_obsluzonych_klientow % 6;
      if (modulo == 0 || modulo == 1 || modulo == 2)
      {
        wskazany_salon = &salony[0];
      }
      else if (modulo == 3 || modulo == 4)
      {
        wskazany_salon = &salony[1];
      }
      else
      {
        wskazany_salon = &salony[2];
      }
      ilosc_obsluzonych_klientow++;

      // Przekieruj klienta do odpowiedniego salonu
      printf("Portier: kieruje klienta do salonu...\n");
      sem_post(&bilet_do_salonu);

      // Czekaj na potwierdzenie klienta że odebrał wiadomość
      printf("Portier: czeka na potwierdzenie ze odebrano bilet...\n");
      sem_wait(&odczytany_bilet_do_salonu);
    }
  }

  pthread_exit(NULL);
}


void* golibrodaFun(void* t)
{
  pthread_t golibroda_id = pthread_self();

  // Rozpocznij pracę w salonie
  salon* obslugiwany_salon = (salon*) t;
  printf("Golibroda %u: Rozpoczyna pracę w salonie nr %d\n", (unsigned)golibroda_id, (*obslugiwany_salon).id);

  while(1)
  {
    // Czekaj na klienta
    printf("Golibroda %u: Czeka na nowego klienta\n", (unsigned)golibroda_id);
    sem_wait(&(*obslugiwany_salon).nowy_klient_w_poczekalni);

    // Czekaj na dostęp do poczekalni
    printf("Golibroda %u: Czeka na dostęp do poczekalni\n", (unsigned)golibroda_id);
    sem_wait(&(*obslugiwany_salon).dostep_do_poczekalni);

    // Wejdź do poczekalni
    printf("Golibroda %u: Wchodzi do poczekalni i bierze klienta\n", (unsigned)golibroda_id);
    (*obslugiwany_salon).zajete_miejsca--;
    sem_post(&(*obslugiwany_salon).wolny_golibroda);

    // Poinformuj, że można znów korzystać z poczekalni
    printf("Golibroda %u: Zwalnia dostęp do poczekalni\n", (unsigned)golibroda_id);
    sem_post(&(*obslugiwany_salon).dostep_do_poczekalni);
  }

  pthread_exit(NULL);
}


int main()
{
  // Inicjuj semafory
  sem_init(&przyszedl_nowy_klient, 0, 0);
  sem_init(&bilet_do_salonu, 0, 0);
  sem_init(&odczytany_bilet_do_salonu, 0, 0);
  sem_init(&klient_obsluzony, 0, 0);

  // Poinformuj o otwarciu salonów
  printf("Otwieramy salony!\n");

  // Inicjuj wątek portiera
  pthread_t portierWatek;
  pthread_create(&portierWatek, NULL, portierFun, NULL);

  // Salon 1
  salony[0].id = 0;
  salony[0].ilosc_miejsc = 9;
  salony[0].zajete_miejsca = 0;
  sem_init(&salony[0].dostep_do_poczekalni, 0, 1);
  sem_init(&salony[0].nowy_klient_w_poczekalni, 0, 0);
  sem_init(&salony[0].wolny_golibroda, 0, 0);
  // Salon 2
  salony[1].id = 1;
  salony[1].ilosc_miejsc = 6;
  salony[1].zajete_miejsca = 0;
  sem_init(&salony[1].dostep_do_poczekalni, 0, 1);
  sem_init(&salony[1].nowy_klient_w_poczekalni, 0, 0);
  sem_init(&salony[1].wolny_golibroda, 0, 0);
  // Salon 3
  salony[2].id = 2;
  salony[2].ilosc_miejsc = 3;
  salony[2].zajete_miejsca = 0;
  sem_init(&salony[2].dostep_do_poczekalni, 0, 1);
  sem_init(&salony[2].nowy_klient_w_poczekalni, 0, 0);
  sem_init(&salony[2].wolny_golibroda, 0, 0);

  // Inicjuj wątki golibrod
  pthread_t golibrodaWatek[6];
  pthread_create(&golibrodaWatek[0], NULL, golibrodaFun, &salony[0]);
  pthread_create(&golibrodaWatek[1], NULL, golibrodaFun, &salony[0]);
  pthread_create(&golibrodaWatek[2], NULL, golibrodaFun, &salony[0]);
  pthread_create(&golibrodaWatek[3], NULL, golibrodaFun, &salony[1]);
  pthread_create(&golibrodaWatek[4], NULL, golibrodaFun, &salony[1]);
  pthread_create(&golibrodaWatek[5], NULL, golibrodaFun, &salony[2]);

  // Inicjuj wątki klientów
  int n = 100;
  pthread_t klienciWatek[n];
  for (int i=0; i<n; i++)
    pthread_create(&klienciWatek[i], NULL, klientFun, NULL);


  pthread_join(portierWatek, NULL);

  return 0;
}


