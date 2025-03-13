#include <stdio.h>
#include <string.h>
#include "tcp.h"
#include "network_sim.h"

int main() {
    network_init(1);

    char* data =
        "Sed ut perspiciatis, unde omnis iste natus error sit voluptatem \n"
        "accusantium doloremque laudantium, totam rem aperiam eaque ipsa, \n"
        "quae ab illo inventore veritatis et quasi architecto beatae vitae \n"
        "dicta sunt, explicabo. Nemo enim ipsam voluptatem, quia voluptas \n"
        "sit, aspernatur aut odit aut fugit, sed quia consequuntur magni \n"
        "dolores eos, qui ratione voluptatem sequi nesciunt, neque porro \n"
        "quisquam est, qui dolorem ipsum, quia dolor sit amet consectetur \n"
        "adipisci[ng] velit, sed quia non numquam [do] eius modi tempora \n"
        "inci[di]dunt, ut labore et dolore magnam aliquam quaerat \n"
        "voluptatem. Ut enim ad minima veniam, quis nostrum[d] exercitationem \n"
        "ullam corporis suscipit laboriosam, nisi ut aliquid ex ea commodi \n"
        "consequatur? [D]Quis autem vel eum i[r]ure reprehenderit, qui in ea \n"
        "voluptate velit esse, quam nihil molestiae consequatur, vel illum, \n"
        "qui dolorem eum fugiat, quo voluptas nulla pariatur?\n"
        "\n"
        "At vero eos et accusamus et iusto odio dignissimos ducimus, qui \n"
        "blanditiis praesentium voluptatum deleniti atque corrupti, quos \n"
        "dolores et quas molestias excepturi sint, obcaecati cupiditate non \n"
        "provident, similique sunt in culpa, qui officia deserunt mollitia \n"
        "animi, id est laborum et dolorum fuga. Et harum quidem reru[d]um \n"
        "facilis est e[r]t expedita distinctio. Nam libero tempore, cum \n"
        "soluta nobis est eligendi optio, cumque nihil impedit, quo minus id, \n"
        "quod maxime placeat facere possimus, omnis voluptas assumenda est, \n"
        "omnis dolor repellend[a]us. Temporibus autem quibusdam et aut \n"
        "officiis debitis aut rerum necessitatibus saepe eveniet, ut et \n"
        "voluptates repudiandae sint et molestiae non recusandae. Itaque \n"
        "earum rerum hic tenetur a sapiente delectus, ut aut reiciendis \n"
        "voluptatibus maiores alias consequatur aut perferendis doloribus \n"
        "asperiores repellat.\n";

    tcp_send(data, strlen(data));

    char* res = network_finalize();
    printf("%s\n", res);
    printf("Correct if this number is zero: %d\n", strcmp(data, res));
    free(res);
}
